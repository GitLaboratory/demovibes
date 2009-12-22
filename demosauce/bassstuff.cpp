/* 
All boost interaction should happen through this file.
To keep it simple, no fancy obejct-stuff atm. Where convenient, I kept the bass types, esp. DWORD.
*/

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

#include "globals.h"

#include "bassstuff.h"

using namespace std;
using namespace boost;

DWORD source = 0;
DecoderType decoder = nada;
HENCODE encoder = 0;
DWORD samplerate = 44100; // just fall back value. should be overwritten by Boost_Init
SongInfo info; // no extended initializer until c++0x :(

void LogBassError()
{
	logg << "ERROR: bass error code: " << BASS_ErrorGetCode() << endl;	
}

void Start();

// encoder death notification
void CALLBACK EncoderNotify(HENCODE handle, DWORD status, void *user)
{
	if (status < 0x10000) { // encoder/connection died
		if (!BASS_Encode_Stop(source)) // free the recording and encoder
		{
			logg << "ERROR: failed to stop encoder\n";
			LogBassError();
			exit(666);
		}
		source = 0;
		bool deadUplink = status == BASS_ENCODE_NOTIFY_CAST;
		logg << (deadUplink ? "ERROR: The server connection died!\n" : "ERROR: The encoder died!\n");
		this_thread::sleep(deadUplink ? posix_time::seconds(60) : posix_time::seconds(1));
		Start(); // restart
	}
}

void FreeSource()
{
	if (source == 0)
		return;
	BOOL ret = TRUE;
	switch (decoder)
	{
		case codec_generic:
			ret = BASS_StreamFree(source);
			break;
		case module_generic:
		case module_amiga:
			ret = BASS_MusicFree(source);
			break;
		default:;
	}
	if (!ret)
	{
		logg << "WARNING: failed to free source\n";
	}
	source = 0;
}

DWORD GetSource()
{  
	FreeSource();
	int errorCount = 0;
	DWORD const module_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN;
	while (source == 0 && errorCount++ <= 3)
	{
		info = GetSong();
		decoder = DecideDecoderType(info.fileName);
		char const * fileName = info.fileName.c_str();
		switch (decoder)
		{
		case codec_generic:
			source = BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_DECODE);  
			//HSTREAM inStream = BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_AUTOFREE);
			//source = BASS_FX_TempoCreate(inStream, BASS_STREAM_AUTOFREE);
			//if (!BASS_ChannelSetAttribute(inStream, BASS_ATTRIB_TEMPO_FREQ, SAMPLERATE))
			//    printf("error on resampler init\n");
			break;    
		case module_generic:
			source = BASS_MusicLoad(FALSE, fileName, 0, 0 , module_flags,samplerate);
			break;
		case module_amiga:
			source = BASS_MusicLoad(FALSE, fileName, 0, 0 , module_flags, samplerate);
			break;    
		default:
			logg << "WARNING: no decoder"  << fileName << endl;
		}
		if (source == 0)
		{
			LogBassError();
		}
		else
		{
			const QWORD pos = BASS_ChannelGetLength(source, BASS_POS_BYTE);
			if (pos != static_cast<QWORD>(-1))
			{
				const WORD length = BASS_ChannelBytes2Seconds(source, pos);
				logg << format("INFO: time length %1% seconds\n") % length;
			}
			else
			{
				logg << "ERROR: no time length " << fileName << endl;
				FreeSource();
			}
		}
	}
	if (errorCount >= 3)
	{
		logg << "ERROR: too many errors, aborting\n";
		exit(666);
	}
	return source;
}

void Start()
{
	// setup encoder command-line (raw PCM data to avoid length limit)
	const DWORD bitrate = lexical_cast<DWORD>(setting::encoder_bitrate);
	char com[100];
	sprintf(com, "lame -r -s %s -b %d -", setting::encoder_samplerate.c_str(), bitrate); // add "-x" for LAME versions pre-3.98

	source = GetSource();
	const DWORD flags = BASS_ENCODE_NOHEAD | BASS_ENCODE_AUTOFREE;
	logg <<  "INFO: starting encoder: " <<  com << endl;
	encoder = BASS_Encode_Start(source, com, flags, NULL, 0); // start the encoder
	if (!encoder) 
	{
		logg << "ERROR: couldn't start encoder\n";
		LogBassError();
		exit(666);
	}
	
	// setup cast
	const char * host = setting::cast_host.c_str();
	const char * pass = setting::cast_password.c_str();
	const char * content = BASS_ENCODE_TYPE_MP3;
	const char * name = setting::cast_name.c_str();
	const char * url = setting::cast_url.c_str();
	const char * genre = setting::cast_genre.c_str();
	const char * desc = setting::cast_description.c_str();
	if (!BASS_Encode_CastInit(encoder, host, pass, content, name, url, genre, desc, NULL, bitrate, TRUE)) 
	{
		logg << "ERROR: couldn't setup connection with server\n";
		logg << format("ERROR: encoder=%1%\n\thost=%2%\n\tpass=%3%\n\tcontent=%4%\n\tname=%5%\n\t" \
			"url=%6%\n\tgenre=%7%\n\tdesc=%8%\n\t bitrate=%9%\n") % encoder % host % pass % content % 
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

bool ChangeSong()
{
	source = GetSource();
	if (!BASS_Encode_SetChannel(encoder, source))
	{
		logg << format("ERROR: BASS_Encode_SetChannel(encoder=%1%, source%=2%)") % encoder % source;
		LogBassError();
		exit(666);
	}
	BASS_Encode_CastSetTitle(encoder, info.title.c_str(), NULL);
	return true;
}

void Bass_Init()
{
	samplerate = lexical_cast<DWORD>(setting::encoder_samplerate);
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0); // thread stuff
	if (!BASS_Init(0, samplerate, 0, 0, NULL)) // 0=no sound device, freq, no flags, console app, no clsid
	{
		logg << "ERROR: BASS_Init failed\n";
		LogBassError();
		exit(666);
	}
	Start();
}
