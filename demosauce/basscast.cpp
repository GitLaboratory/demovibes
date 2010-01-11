#include <cstdlib>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/static_assert.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

#include "globals.h"
#include "basssource.h"
#include "sockets.h"
#include "basscast.h"

using namespace std;
using namespace boost;
using namespace logror;

enum DecoderType
{
	nada,
	noise,
	codec_generic,
	module_generic,
	module_amiga
};
DecoderType activeDecoder = nada;
DWORD (* ActiveFillBuffer) (void *, DWORD) = NULL;
HENCODE encoder = 0;
HSTREAM sink = 0;
size_t noiseBytes = 0;

DecoderType DecideDecoderType(const string & fileName)
{
	DecoderType type = nada;
	if (iends_with(fileName, ".mp3") ||
		iends_with(fileName, ".mp2") ||
		iends_with(fileName, ".mp1") ||
		iends_with(fileName, ".ogg") ||
		iends_with(fileName, ".aac") ||
		iends_with(fileName, ".m4a"))
		type = codec_generic;
	else if (iends_with(fileName, ".xm") ||
		iends_with(fileName, ".s3m") ||
		iends_with(fileName, ".it"))
		type = module_generic;
	else if (iends_with(fileName, ".mod"))
		type = module_amiga;
	else
		Error("no decoder for %1%"), fileName;
	return type;
}

size_t NoiseFillBuffer(void * buffer, size_t length)
{ 
	BOOST_STATIC_ASSERT(SAMPLE_SIZE == 2);
	if (length < SAMPLE_SIZE * setting::encoder_channels)
		return 0;
	const size_t bytesToWrite = min(noiseBytes, length);
	int16_t * out = reinterpret_cast<int16_t *>(buffer);
	for (size_t i = 0; i < bytesToWrite / SAMPLE_SIZE; ++i)
		*out++ = static_cast<int16_t>(rand() & 0x00ff);
	noiseBytes -= bytesToWrite;
	return bytesToWrite;
}

// this is called whenever the song is changed
void ChangeSong()
{
	// free old decoder
	switch (activeDecoder)
	{
		case codec_generic:
			BassSourceFreeStream();
			break;    
		case module_generic:
		case module_amiga:
			BassSourceFreeMusic();
			break;
		default:;
	}
	activeDecoder = nada;
	SongInfo songInfo;
	// find new decoder
	while (activeDecoder == nada)
	{
		songInfo = GetNextSong();
		activeDecoder = DecideDecoderType(songInfo.fileName);
		switch (activeDecoder)
		{
			case codec_generic:
				if (!BassSourceLoadStream(songInfo.fileName))
					activeDecoder = nada;
				break;    
			case module_generic:
			case module_amiga:
				if (!BassSourceLoadMusic(songInfo.fileName))
					activeDecoder = nada;
				break;
		default:;
		}
		if (activeDecoder == nada && songInfo.fileName == setting::error_tune)
		{
			activeDecoder = noise;
			noiseBytes = setting::encoder_samplerate * setting::encoder_channels * SAMPLE_SIZE * 120; // 2 minutes
			Log(warning, "could not play error_tune, playing some noise instead");
		}
	}
	// assign active decode function
	switch (activeDecoder)
	{
		case noise:
			ActiveFillBuffer = NoiseFillBuffer;
		break;
		case codec_generic:
			ActiveFillBuffer = BassSourceFillBufferStream;
			break;
		case module_generic:
		case module_amiga:
			ActiveFillBuffer = BassSourceFillBufferMusic;
			break;
		default:
			Fatal("grill your coder");
	}	
	BASS_Encode_CastSetTitle(encoder, songInfo.title.c_str(), NULL);
}

// this is where most of the shit happens
DWORD CALLBACK FillBuffer(HSTREAM handle, void * buffer, DWORD length, void * user)
{
	if (ActiveFillBuffer == NULL)
	{
		Log(warning, "ActiveFillBuffer is NULL");
		memset(buffer, 0, length);
		ChangeSong();
		return length;
	}
	const DWORD bytesWritten = numeric_cast<DWORD>(ActiveFillBuffer(buffer, length));
	if (bytesWritten > length)
		Fatal("WTF!? possible buffer overrun? quitting shitting my pants");
	if (bytesWritten < length) // should indicate end of source file, needs to be testted irl
	{
		Log(info, "end of source, %1% bytes remaining"), bytesWritten;
		memset(buffer, 0, length - bytesWritten); // fill rest of buffer with zeros. i'm way too lazy to mangle buffers at this point
		ChangeSong();
	}
	return length;
}

void Start();

// encoder death notification
void CALLBACK EncoderNotify(HENCODE handle, DWORD status, void *user)
{
	if (status < 0x10000) 
	{ 	// encoder/connection died
		if (!BASS_Encode_Stop(encoder)) // free the recording and encoder
			Log(warning, "failed to stop old encoder %1%"), BASS_ErrorGetCode();
		bool deadUplink = status == BASS_ENCODE_NOTIFY_CAST;
		if (deadUplink)
			Error("the server connection died");
		else
			Error("the encoder died");
		this_thread::sleep(deadUplink ? posix_time::seconds(60) : posix_time::seconds(1));
		Start(); // restart
	}
}

void Start()
{
	// set up source stream -- samplerate, number channels, flags, callback, user data
	sink = BASS_StreamCreate(setting::encoder_samplerate, setting::encoder_channels, BASS_STREAM_DECODE, &FillBuffer, NULL);
	if (!sink)
		Fatal("couldn't create sink (%1%)"), BASS_ErrorGetCode();
	// setup encoder 
	string command = str(format("lame -r -s %1% -b %2% -") % setting::encoder_samplerate % setting::encoder_bitrate);
	Log(info, "starting encoder %1%"), command;
	encoder = BASS_Encode_Start(sink, command.c_str(), BASS_ENCODE_NOHEAD | BASS_ENCODE_AUTOFREE, NULL, 0);
	if (!encoder) 
		Fatal("couldn't start encoder (%1%)"), BASS_ErrorGetCode();
	// setup casting
	string host = setting::cast_host; //
	ResolveIp(setting::cast_host, host); // bass didn't resolve localhost, but if this fails we try anyways
	string server = str(format("%1%:%2%/%3%") % host % setting::cast_port % setting::cast_mount);
	const char * pass = setting::cast_password.c_str();
	const char * content = BASS_ENCODE_TYPE_MP3;
	const char * name = setting::cast_name.c_str();
	const char * url = setting::cast_url.c_str();
	const char * genre = setting::cast_genre.c_str();
	const char * desc = setting::cast_description.c_str(); 
	const DWORD bitrate = setting::encoder_bitrate;
	//NULL = no additional headers, TRUE = make public ok(list at shoutcast?)
	if (!BASS_Encode_CastInit(encoder, server.c_str(), pass, content, name, url, genre, desc, NULL, bitrate, TRUE)) 
		Fatal("couldn't set up connection with icecast (%1%)\n\tserver=%2%\n\tpass=%3%\n\tcontent=%4%\n\tname=%5%\n" \
			"\turl=%6%\n\tgenre=%7%\n\tdesc=%8%\n\tbitrate=%9%"), BASS_ErrorGetCode(), server, pass, content, name, 
			url,genre,desc, bitrate;
	Log(info, "connected to icecast %1%"), server;
	if (!BASS_Encode_SetNotify(encoder, EncoderNotify, 0)) // notify of dead encoder/connection
		Fatal("couldn't set callback cunction (%1%)"), BASS_ErrorGetCode();
}

void BassCastInit()
{
	// 0=no sound device, freq, no flags, console app, no clsid
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	if (!BASS_Init(0, setting::encoder_samplerate, 0, 0, NULL)) 
		Fatal("BASS init failed (%1%)"), BASS_ErrorGetCode();
	ChangeSong();
	Start();
}

void BassCastRun()
{   
    const size_t buffSize = numeric_cast<size_t>(setting::encoder_samplerate);
    char buff[buffSize]; // don't like buffers on stack 
	for (;;)
	{
		// this should block once the streamer can't keep up
		const DWORD bytesRead = BASS_ChannelGetData(sink, buff, buffSize); 
    	if (bytesRead == static_cast<DWORD>(-1))
			Fatal("lost sink channel (%1%)"), BASS_ErrorGetCode();
	}
}
