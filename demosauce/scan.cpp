#include <cstdlib>

#include <cstring>
#include <string>
#include <iostream>

#include "bass/bass.h"
#include "bass/bass_aac.h"
#include "bass/bassflac.h"

#include "libreplaygain/replay_gain.h"

#include "logror.h"
#include "misc.h"
#include "basssource.h"
#include "avsource.h"

std::string BassTypeToString(DWORD channelType)
{
	switch (channelType)
	{
		case BASS_CTYPE_STREAM_OGG: return "ogg";
		case BASS_CTYPE_STREAM_MP1: return "mp1";
		case BASS_CTYPE_STREAM_MP2: return "mp2";
		case BASS_CTYPE_STREAM_MP3: return "mp3";
		case BASS_CTYPE_STREAM_AIFF: return "aiff";
		case BASS_CTYPE_STREAM_CA: return "ca";
		case BASS_CTYPE_STREAM_WAV_PCM:
		case BASS_CTYPE_STREAM_WAV_FLOAT:
		case BASS_CTYPE_STREAM_WAV: return "wav";
		case BASS_CTYPE_STREAM_AAC: return "aac";
		case BASS_CTYPE_STREAM_MP4: return "mp4";
		case BASS_CTYPE_STREAM_FLAC: return "flac";
		case BASS_CTYPE_STREAM_FLAC_OGG: return "flac-ogg";
		
		case BASS_CTYPE_MUSIC_MOD: return "mod";
		case BASS_CTYPE_MUSIC_MTM: return "mtm";
		case BASS_CTYPE_MUSIC_S3M: return "s3m";
		case BASS_CTYPE_MUSIC_XM: return "xm";
		case BASS_CTYPE_MUSIC_IT: return "it";
		case BASS_CTYPE_MUSIC_MO3: return "mo3";
		default: return "unknown";
	}
}

int main(int argc, char* argv[])
{
	logror::LogSetConsoleLevel(logror::fatal);
	//logror::LogSetConsoleLevel(logror::debug);
	if (argc < 2 || (*argv[1] == '-' && argc < 3)) 
		logror::Fatal("not enough arguments");
	const char * fileName = argv[argc - 1];
	
	// open file
	BassSource bassSource;
	if (!bassSource.Load(fileName, true))
	{
		if (BASS_ErrorGetCode() == BASS_ERROR_NOTAVAIL)
			logror::Fatal("unable to determine length");
		else
			logror::Fatal("failed to load file (code: %1%)"), BASS_ErrorGetCode();
	}
	if (bassSource.Channels() < 1 || bassSource.Channels() > 2)
		logror::Fatal("usupported number of channels");

	std::cout << "type:" << BassTypeToString(bassSource.BassChannelType()) << std::endl;
	std::cout << "length:" << bassSource.Duration() << std::endl;
	std::cout << "bitrate:" << bassSource.Bitrate() << std::endl;
	std::cout << "loopiness:" << bassSource.Loopiness() << std::endl;
	
	if (strcmp(argv[1], "--no-replay_gain"))
	{
		RG_SampleFormat format;
		format.sampleRate = bassSource.Samplerate();
		format.sampleType = RG_FLOAT_32_BIT;
		format.numberChannels = bassSource.Channels();
		format.interleaved = FALSE;
		RG_Context * context = RG_NewContext(&format);
		float buffer[48000 * 4];
		static uint32_t const buffFrames = 	BytesInFrames<uint32_t, float>(sizeof(buffer), bassSource.Channels());
		uint32_t frames = 0;
		while ((frames = bassSource.Process(buffer, buffFrames)) == buffFrames)	
			RG_Analyze(context, buffer, frames);
		std::cout << "replaygain:" << RG_GetTitleGain(context) << std::endl;
		RG_FreeContext(context);
	}
	return EXIT_SUCCESS;
}
