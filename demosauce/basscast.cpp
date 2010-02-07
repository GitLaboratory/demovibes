#include <cstring>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

#include "globals.h"
#include "sockets.h"
#include "misc.h"
#include "dsp.h"
#include "convert.h"
#include "basssource.h"
#include "avsource.h"
#include "basscast.h"

using namespace std;
using namespace boost;
using namespace logror;

/*	current processing stack layout
	NoiseSource / BassSource / AVCodecSource -> (Resample) -> (MixChannels) -> 
	-> MapChannels -> (LinearFade) -> Gain -> (LinearClipper) -> BassCast
*/

typedef int16_t sample_t;

struct BassCastPimpl
{
	void Start();
	void ChangeSong();
	void InitMachines();
	//-----------------
	Sockets sockets;
	ConvertToInterleaved converter;
	shared_ptr<MachineStack> machineStack;
	shared_ptr<NoiseSource> noiseSource;
	shared_ptr<BassSource> bassSource;
	shared_ptr<Resample> resample;
	shared_ptr<AvSource> avSource;
	shared_ptr<MixChannels> mixChannels;
	shared_ptr<MapChannels> mapChannels;
	shared_ptr<LinearFade> linearFade;
	shared_ptr<Gain> gain;
	shared_ptr<Peaky> peaky;
	//shared_ptr<Brickwall> brickwall;
	HENCODE encoder;
	HSTREAM sink;
	vector<float> readBuffer;
	//-----------------
	BassCastPimpl() :
		sockets(setting::demovibes_host, setting::demovibes_port),
		encoder(0),
		sink(0) 
		{
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
			if (!BASS_Init(0, setting::encoder_samplerate, 0, 0, NULL) &&
				BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
				Fatal("BASS init failed (%1%)"), BASS_ErrorGetCode();
			InitMachines();
			ChangeSong();	
			Start();
		}
};

BassCast::BassCast() : pimpl(new BassCastPimpl) { }

template <typename T> inline shared_ptr<T> new_shared()
{ return shared_ptr<T>(new T); }

void BassCastPimpl::InitMachines()
{
	machineStack = new_shared<MachineStack>();
	noiseSource = new_shared<NoiseSource>();
	bassSource = new_shared<BassSource>();
//	avSource = new_shared<AvSource>();
	resample = new_shared<Resample>();
	mixChannels = new_shared<MixChannels>();
	mapChannels = new_shared<MapChannels>();
	linearFade = new_shared<LinearFade>();
	gain = new_shared<Gain>();
	peaky = new_shared<Peaky>();
//	brickwall = new_shared<Brickwall>();
	
	float ratio = setting::amiga_channel_ratio; 
	mixChannels->Set(1 - ratio, ratio, 1 - ratio, ratio);
	mapChannels->SetOutChannels(setting::encoder_channels);
	
	machineStack->AddMachine(noiseSource);
	machineStack->AddMachine(resample);
	machineStack->AddMachine(mixChannels);
	machineStack->AddMachine(mapChannels);
	machineStack->AddMachine(linearFade);
	machineStack->AddMachine(gain);
//	machineStack->AddMachin(brickwall);
	machineStack->AddMachine(peaky);
	
	converter.SetSource(machineStack);
}

void BassCast::Run()
{   
    const size_t buffSize = setting::encoder_samplerate * 2;
    char * buff = new char[buffSize]; // don't like buffers on stack 
	for (;;)
	{
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
	resample->SetBypass(true);
	mixChannels->SetBypass(true);
	linearFade->SetBypass(true);
	LogDebug("last peak: %1%"), peaky->Peak();
	peaky->Reset();
	SongInfo songInfo;
	for (bool loadSuccess = false; !loadSuccess;)
	{
		if (setting::debug_file.size() > 0)
			songInfo.fileName = setting::debug_file;
		else
			sockets.GetSong(songInfo);
		
		if (!boost::filesystem::exists(songInfo.fileName))
		{	
			Error("file doesn't exist: %1%"), songInfo.fileName;
			continue;
		}
		
		bool bassLoaded = false;
		bool avLoaded = false;
		
		// educated guess
		if (BassSource::CheckExtension(songInfo.fileName))
			bassLoaded = bassSource->Load(songInfo.fileName);
//		else
//		if (AvSource::CheckExtension(songInfo.fileName))
//			avLoaded = avSource->Load(songInfo.fileName);
		
		// brute force
//		if (!bassLoaded && !avcodecLoaded)
//			avLoaded = avSource->Load(songInfo.fileName);
		if (!bassLoaded && !avLoaded)
			bassLoaded = bassSource->Load(songInfo.fileName);
		
		if (bassLoaded)
		{
			if (bassSource->Samplerate() != setting::encoder_samplerate)
			{
				resample->Set(bassSource->Samplerate(), setting::encoder_samplerate);
				resample->SetBypass(false);
			}
			if (bassSource->IsAmigaModule() && setting::encoder_channels == 2)
				mixChannels->SetBypass(false);
			if (songInfo.loopDuration > 0)
			{
				bassSource->SetLoopDuration(songInfo.loopDuration);
				uint64_t const start = (songInfo.loopDuration - 5) * setting::encoder_samplerate;
				uint64_t const end = songInfo.loopDuration * setting::encoder_samplerate;
				linearFade->Set(start, end, 1, 0);
				linearFade->SetBypass(false);
			}
			machineStack->AddMachine(bassSource, 0);
			Log(info, "duration %1% seconds"), bassSource->Duration();
			loadSuccess = true;
		}
		
// 		if (avLoaded)
// 		{
// 			machineStack->AddMachine(avSource, 0);
// 			Log(info, "duration %1% seconds"), avSource->Duration();
// 			loadSuccess = true;
// 		}
		
		if (!loadSuccess && songInfo.fileName == setting::error_tune)
		{
			Log(warning, "no error tune, playing 2 minutes of glorious noise"), songInfo.fileName;
			noiseSource->Set(setting::encoder_channels, 120 * setting::encoder_samplerate);
			gain->SetAmp(DbToAmp(-12));
			machineStack->AddMachine(noiseSource, 0);
			loadSuccess = true;
		}
	}
	
	// once clipping is working also apply positive gain
	gain->SetAmp(DbToAmp(songInfo.gain > 0 ? 0: songInfo.gain));
	machineStack->UpdateRouting();
	BASS_Encode_CastSetTitle(encoder, songInfo.title.c_str(), NULL);
}

// this is where most of the shit happens
DWORD FillBuffer(HSTREAM handle, void * buffer, DWORD length, void * user)
{
	BassCastPimpl & pimpl = * reinterpret_cast<BassCastPimpl*>(user);
	uint32_t const channels = setting::encoder_channels;
	uint32_t const frames = BytesInFrames<uint32_t, sample_t>(length, channels);
	sample_t * const outBuffer = reinterpret_cast<sample_t*>(buffer);
	uint32_t const framesRead = pimpl.converter.Process(outBuffer, frames);

	if (framesRead > frames)
		Fatal("WTF!? possible buffer overrun? quitting shitting my pants");
	
	if (framesRead < frames) // implicates end of stream
	{
		Log(info, "end of source, %1% frames remaining"), frames - framesRead;
		size_t const bytesRead = FramesInBytes<sample_t>(framesRead, channels);
		memset(reinterpret_cast<char*>(buffer) + bytesRead, 0, length - bytesRead);
		pimpl.ChangeSong();
	}
	return length;
}

// encoder death notification
void EncoderNotify(HENCODE handle, DWORD status, void * user)
{
	BassCastPimpl & pimpl = * reinterpret_cast<BassCastPimpl*>(user);
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
