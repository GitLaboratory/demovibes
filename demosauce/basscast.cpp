#include <cstring>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

#include "globals.h"
#include "sockets.h"
#include "dsp.h"
#include "basssource.h"
#include "basscast.h"

using namespace std;
using namespace boost;
using namespace logror;

/*	current processing stack layout, () machines can be bypassed
	NoiseSource -> (BassSource) -> MapChannels -> Gain -> BassCast
	planned layout:
	NoiseSource -> (BassSource) -> (Resample) -> (MixChannels) -> MapChannels ->
	(LinearFade) -> Gain -> BassCast
*/

struct BassCastPimpl
{
	void Start();
	void ChangeSong();
	//-----------------
	Sockets sockets;
	MachineStack machineStack;
	shared_ptr<BassSource> bassSource;
	shared_ptr<NoiseSource> noiseSource;
	shared_ptr<MapChannels> mapChannels;
	shared_ptr<Gain> gain;
	HENCODE encoder;
	HSTREAM sink;
	//-----------------
	BassCastPimpl() :
		sockets(setting::demovibes_host, setting::demovibes_port),
		bassSource(new BassSource),
		noiseSource(new NoiseSource),
		mapChannels(new MapChannels),
		gain(new Gain),
		encoder(0),
		sink(0) 
		{
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
			if (!BASS_Init(0, setting::encoder_samplerate, 0, 0, NULL) &&
				BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
				Fatal("BASS init failed (%1%)"), BASS_ErrorGetCode();
			mapChannels->SetOutChannels(setting::encoder_channels);
			machineStack.AddMachine(noiseSource);
			machineStack.AddMachine(bassSource);
			machineStack.AddMachine(mapChannels);
			machineStack.AddMachine(gain);
			machineStack.UpdateRouting();
			ChangeSong();	
			Start();
		}
};

BassCast::BassCast() : pimpl(new BassCastPimpl) { }

void BassCast::Run()
{   
    const size_t buffSize = numeric_cast<size_t>(setting::encoder_samplerate * 2);
    char * buff = new char[buffSize]; // don't like buffers on stack 
	for (;;)
	{
		// this should block once the streamer can't keep up
		const DWORD bytesRead = BASS_ChannelGetData(pimpl->sink, buff, buffSize); 
    	if (bytesRead == static_cast<DWORD>(-1))
    	{
    		delete buff;
			Fatal("lost sink channel (%1%)"), BASS_ErrorGetCode();
		}
	}
	delete buff; // never reached, but what the hell..
}

// this is called whenever the song is changed
void BassCastPimpl::ChangeSong()
{
	// reset routing
	bassSource->SetBypass(false);
	SongInfo songInfo;
	for (bool loadSuccess = false; !loadSuccess;)
	{
		sockets.GetSong(songInfo);
		loadSuccess = bassSource->Load(songInfo.fileName);
		gain->SetAmp(songInfo.gain);
		if (!loadSuccess && songInfo.fileName == setting::error_tune)
		{
			Log(warning, "no error tune, playing 2 minutes of glorious noise"), songInfo.fileName;
			noiseSource->Set(setting::encoder_channels, 120 * setting::encoder_samplerate);
			bassSource->SetBypass(true);
			loadSuccess = true;
		}
	}
	machineStack.UpdateRouting();
	BASS_Encode_CastSetTitle(encoder, songInfo.title.c_str(), NULL);
}

// this is where most of the shit happens
DWORD FillBuffer(HSTREAM handle, void * buffer, DWORD length, void * user)
{
	BassCastPimpl & pimpl = * reinterpret_cast<BassCastPimpl*>(user);
	uint32_t const & channels = setting::encoder_channels;
	uint32_t framesWritten = 0;
	uint32_t const frames = bytes2frames<uint32_t, int16_t>(length, channels);
	framesWritten = pimpl.machineStack.Process(reinterpret_cast<int16_t*>(buffer), frames);
	if (framesWritten > frames)
		Fatal("WTF!? possible buffer overrun? quitting shitting my pants");
	if (framesWritten < frames) // implicates end of stream
	{
		Log(info, "end of source, %1% frames remaining"), frames - framesWritten;
		size_t const bytesWritten = frames2bytes<size_t, int16_t>(framesWritten, channels);
		memset(reinterpret_cast<char*>(buffer) + bytesWritten, 0, length - bytesWritten);
		pimpl.ChangeSong();
	}
	return length;
}

// encoder death notification
void EncoderNotify(HENCODE handle, DWORD status, void * user)
{
	BassCastPimpl & pimpl = * reinterpret_cast<BassCastPimpl *>(user);
	if (status >= 0x10000) 
		return;
	if (!BASS_Encode_Stop(pimpl.encoder)) 
		Log(warning, "failed to stop old encoder %1%"), BASS_ErrorGetCode();
	bool deadUplink = status == BASS_ENCODE_NOTIFY_CAST;
	if (deadUplink)
		Error("the server connection died");
	else
		Error("the encoder died");
	this_thread::sleep(deadUplink ? posix_time::seconds(60) : posix_time::seconds(1));
	pimpl.Start();
}

void 
BassCastPimpl::Start()
{
	// set up source stream -- samplerate, number channels, flags, callback, user data
	sink = BASS_StreamCreate(setting::encoder_samplerate, setting::encoder_channels, 
		BASS_STREAM_DECODE, &FillBuffer, this);
	if (!sink)
		Fatal("couldn't create sink (%1%)"), BASS_ErrorGetCode();
	// setup encoder 
	string command = str(format("lame -r -s %1% -b %2% -") % setting::encoder_samplerate % setting::encoder_bitrate);
	Log(info, "starting encoder %1%"), command;
	encoder = BASS_Encode_Start(sink, command.c_str(), BASS_ENCODE_NOHEAD | BASS_ENCODE_AUTOFREE, NULL, 0);
	if (!encoder) 
		Fatal("couldn't start encoder (%1%)"), BASS_ErrorGetCode();
	// setup casting
	string host = setting::cast_host; 
	ResolveIp(setting::cast_host, host); // if resolve fails, host will retain it's original value
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

BassCast::~BassCast() {} // this HAS to be here, or scoped_ptr will poop in it's pants, header won't work.
