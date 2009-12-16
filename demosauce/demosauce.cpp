/*
	fancy streaming engine for scenemusic 
	written by maep, [put your names here]  2009 
*/

#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "bass/bass.h"
#include "bass/bassenc.h"

using namespace std;
using namespace boost;
using boost::filesystem::path;
using boost::format;
using boost::io::group;


// global stuff
#define SAMPLERATE 44100
static ofstream log("demosauce.log");
HENCODE encoder;

void LogBassError()
{
    log << format("ERROR: bass error code: %1%\n") % BASS_ErrorGetCode();	
}

void Start();
void Stop();

// encoder death notification
void CALLBACK EncoderNotify(HENCODE handle, DWORD status, void *user)
{
	if (status < 0x10000) { // encoder/connection died
		Stop(); // free the recording and encoder
		bool deadUplink = status == BASS_ENCODE_NOTIFY_CAST;
		log << (deadUplink ? "The server connection died!\n" : "The encoder died!\n");
		sleep(deadUplink ? 60 : 1); // wait for a while
		Start(); // restart
	}
}

enum DecoderType
{
    codec_generic,
    module_generic,
    module_amiga, // 
    nada  
};

DecoderType DecideDecoderType(const path & file)
{
    // neither fast nor elegant, but easy to understand
    DecoderType type = nada;
    const string name = file.filename();
    if (iends_with(name, ".mp3") ||
        iends_with(name, ".mp2") ||
        iends_with(name, ".mp1") ||
        iends_with(name, ".ogg") ||
        iends_with(name, ".aac") ||
        iends_with(name, ".m4a"))
        type = codec_generic;
    else if (iends_with(name, ".xm") ||
        iends_with(name, ".s3m") ||
        iends_with(name, ".it"))
        type = module_generic;
    else if (iends_with(name, ".mod"))
        type = module_amiga;
    return type;
}

// to be removed once streaming & basic control is working
DWORD GetBassSource()
{
    // get filename from python name
    const char * fileName = "";
    return BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_AUTOFREE);  
}

bool SongChange()
{  
    const path file; // get filename from python
    DWORD source = 0;
    char const * const fileName = file.file_string().c_str();
/*    switch (DecideDecoderType(file))
    {
    case codec_generic:;
        source = BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_AUTOFREE);  
        //HSTREAM inStream = BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_AUTOFREE);
        //source = BASS_FX_TempoCreate(inStream, BASS_STREAM_AUTOFREE);
        //if (!BASS_ChannelSetAttribute(inStream, BASS_ATTRIB_TEMPO_FREQ, SAMPLERATE))
        //    printf("error on resampler init\n");
        break;    
    case module_generic:
        source = BASS_MusicLoad(FALSE, fileName, 0, 0 , 0, SAMPLERATE);
        break;
    case module_amiga:
        source = BASS_MusicLoad(FALSE, fileName, 0, 0 , 0, SAMPLERATE);
        break;    
    default:
        log << format("WARNING: no decoder %1%\n") % file.filename();
        return false;
    }
*/  if (!BASS_Encode_SetChannel(encoder, source))
    {
        log << format("BASS_Encode_SetChannel(encoder=%1%, source%=2%)") % encoder % source;
        LogBassError();
        return false;
    }
    return true;
}

void Start()
{
	// setup encoder command-line (raw PCM data to avoid length limit)
	const DWORD bitrate = 160;
	char com[100];
	sprintf(com, "lame -r -s %d -b %d -", SAMPLERATE, bitrate); // add "-x" for LAME versions pre-3.98

    // the current source is just a workaround to get it working
    // i'd like to use a stream whith direct access to the buffers. at that level we can do pretty much 
    // anything. this is where uade or effects will have access to bass
	const DWORD source = GetBassSource(); 
	
	const DWORD flags = BASS_ENCODE_NOHEAD | BASS_ENCODE_AUTOFREE | BASS_ENCODE_PAUSE;
    log << format( "INFO: starting encoder: %1%\n") % com;
	encoder = BASS_Encode_Start(source, com, flags, NULL, 0); // start the encoder

	if (!encoder) 
	{
		log << "ERROR: couldn't start encoding\n";
        LogBassError();
		return;
	}
	
	// setup cast
	const char * server = "scenemusic.eu";
	const char * pass = "doom";
	const char * content = BASS_ENCODE_TYPE_MP3;
	const char * name = "nectarine";
	const char * url = "http://scenemusic.eu/";
	const char * genre = "ship-tune";
	const char * desc = "a nice online radio";
	if (!BASS_Encode_CastInit(encoder, server, pass, content, name, url, genre, desc, NULL, bitrate, TRUE)) 
	{
		log << "ERROR: couldn't setup connection with server\n";
	    log << format("ERROR: encoder=%1%\n\tserver=%2%\n\tpass=%3%\n\tcontent=%4%\n\tname=%5%\n\t" \
	        "url=%6%\n\tgenre=%7%\n\tdesc=%8%\n\t bitrate=%9%") % encoder % server % pass % content % 
	        name % url % genre % desc % bitrate;
		LogBassError();
		return;
	}
	if (!BASS_Encode_SetNotify(encoder, EncoderNotify, 0)) // notify of dead encoder/connection
	{
		log << "ERROR: couldn't set callback cunction\n";
		LogBassError();
		return;
	}
}

void Stop()
{
	BASS_Encode_Stop(encoder);
}

void TitleChanged(char* title)
{
	BASS_Encode_CastSetTitle(encoder, title, NULL);
}

int main(int argc, char* argv[])
{
    // removed gui stuff

    return 0;
}

