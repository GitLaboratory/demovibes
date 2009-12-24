#include <string>

#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

#include "globals.h"

#include "basssource.h"
#include "basscast.h"

using namespace std;
using namespace boost;

enum DecoderType
{
	nada,
	codec_generic,
	module_generic,
	module_amiga
};

DecoderType activeDecoder = nada;
SongInfo info; // no extended initializer until c++0x :(
HENCODE encoder = 0;
HSTREAM sink = 0;

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
		logg << "WARNING: no decoder"  << fileName << endl;
	return type;
}

// this is called whenever the song is changed
void ChangeSong()
{
	switch (activeDecoder)
	{
		case codec_generic:
			BassSourceFreeStream();
			break;    
		case module_generic:
		case module_amiga:
			BassSourceFreeMusic();
			break;
		default:;// swing balls
	}
	activeDecoder = nada;
	int errorCount = 0;
	while (activeDecoder == nada && errorCount++ <= 3)
	{
		info = GetNextSong();
		activeDecoder = DecideDecoderType(info.fileName);
		switch (activeDecoder)
		{
		case codec_generic:
			if (!BassSourceLoadStream(info.fileName))
				activeDecoder = nada;
			break;    
		case module_generic:
		case module_amiga:
			if (!BassSourceLoadMusic(info.fileName))
				activeDecoder = nada;
			break; 
		default:
			logg << "WARNING: no decoder " << info.fileName << endl;			
		}
	}
	if (errorCount >= 3)
	{
		logg << "ERROR: too many errors, aborting\n";
		exit(666);
	}
	BASS_Encode_CastSetTitle(encoder, info.title.c_str(), NULL);
}

// this is where most of the shit happens
DWORD CALLBACK FillBuffer(HSTREAM handle, void * buffer, DWORD length, void *user)
{
	DWORD bytesWritten = 0;
	switch (activeDecoder)
	{
		case codec_generic:
			bytesWritten = BassSourceFillBufferStream(buffer, length);
			break;
		case module_generic:
		case module_amiga:
			bytesWritten = BassSourceFillBufferMusic(buffer, length);
			break;
		default:
			logg << "ADVICE: grill your coder\n"; // should never be reached
			exit(666);
	}
	if (bytesWritten > length)
	{
		logg << "ERROR: WTF!? possible buffer overrun? quitting shitting my pants.\n";
		exit(666);
	}
	if (bytesWritten < length) // should indicate end of source file, needs to be testted irl
	{
		logg << format("INFO: end of source, %1% bytes remaining\n") % bytesWritten; // not shoure what will happen bytes written is zero
		memset(buffer, 0, length - bytesWritten); // fill rest of buffer with zeros. i'm way too lazy to mangle buffers at this point
		ChangeSong();
	}
	return length;
}

void Start();

// encoder death notification
void CALLBACK EncoderNotify(HENCODE handle, DWORD status, void *user)
{
	static int deathCounter = 0;
	if (status < 0x10000) { // encoder/connection died
		if (deathCounter++ > 9)
		{
			logg << "ERROR: too many encoder deaths\n";
			exit(666);
		}
		if (!BASS_Encode_Stop(encoder)) // free the recording and encoder
		{
			logg << "WARNING: failed to stop old encoder\n";
			LogBassError();
		}
		bool deadUplink = status == BASS_ENCODE_NOTIFY_CAST;
		logg << (deadUplink ? "ERROR: The server connection died!\n" : "ERROR: The encoder died!\n");
		this_thread::sleep(deadUplink ? posix_time::seconds(60) : posix_time::seconds(1));
		Start(); // restart
	}
}

void Start()
{
	// set up source stream -- samplerate, number channels, flags, callback, user data
	sink = BASS_StreamCreate(setting::encoder_samplerate_int, 2, BASS_STREAM_DECODE, &FillBuffer, NULL);
	if (!sink)
	{
		logg << "ERROR: couldn't create sink\n";
		LogBassError();
		exit(666);
	}
	// setup encoder 
	string command = str(format("lame -r -s %s -b %s -") % setting::encoder_samplerate % setting::encoder_bitrate);
	const DWORD flags = BASS_ENCODE_NOHEAD | BASS_ENCODE_AUTOFREE;
	logg <<  "INFO: starting encoder: " << command << endl;
	encoder = BASS_Encode_Start(sink, command.c_str(), flags, NULL, 0); // start the encoder
	if (!encoder) 
	{
		logg << "ERROR: couldn't start encoder\n";
		LogBassError();
		exit(666);
	}
	// setup casting
	string server = str(format("%s:%s/%s") % setting::cast_host  % setting::cast_port % setting::cast_mount);
	const char * pass = setting::cast_password.c_str();
	const char * content = BASS_ENCODE_TYPE_MP3;
	const char * name = setting::cast_name.c_str();
	const char * url = setting::cast_url.c_str();
	const char * genre = setting::cast_genre.c_str();
	const char * desc = setting::cast_description.c_str(); 
	const DWORD bitrate = setting::encoder_bitrate_int;
	// in next line: NULL = no additional headers, TRUE = make public(list at shoutcast?)
	if (!BASS_Encode_CastInit(encoder, server.c_str(), pass, content, name, url, genre, desc, NULL, bitrate, TRUE)) 
	{
		logg << "ERROR: couldn't set up connection with server\n";
		logg << format("ERROR:\tserver=%1%\n\tpass=%2%\n\tcontent=%3%\n\tname=%4%\n\t" \
			"url=%5%\n\tgenre=%6%\n\tdesc=%7%\n\tbitrate=%8%\n") % server % pass % content % 
			name % url % genre % desc % bitrate;
		LogBassError();
		exit(666);
	}
	if (!BASS_Encode_SetNotify(encoder, EncoderNotify, 0)) // notify of dead encoder/connection
	{
		logg << "ERROR: couldn't set callback cunction\n";
		LogBassError();
		exit(666);
	}
}

void BassCastInit()
{
	// 0=no sound device, freq, no flags, console app, no clsid
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	if (!BASS_Init(0, setting::encoder_samplerate_int, 0, 0, NULL)) 
	{
		logg << "ERROR: BASS_Init failed\n";
		LogBassError();
		exit(666);
	}
	ChangeSong();
	Start();
}

char testbuff[88200];

void Test()
{
	BASS_ChannelGetData(sink, testbuff, 88200);
}
