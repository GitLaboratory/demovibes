#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

#include "bass/bass.h"
#include "bass/bass_aac.h"
#include "bass/bassflac.h"

#include "replay_gain/replay_gain.h"

#include "logror.h"
#include "decoder_common.h"
#include "basssource.h"

std::string TypeToString(DWORD channelType)
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
	if (argc < 2 || (*argv[1] == '-' && argc < 3)) 
		logror::Fatal("not enough arguments");
	const char * fileName = argv[argc - 1];
	
	// open file
	BassSource source;
	bool success = false;
	switch(DecideDecoderType(fileName))
	{
		case decoder_codec_generic:
		case decoder_codec_aac:
		case decoder_codec_mp4:
		case decoder_codec_flac:
			success = source.Load(fileName, true);
			break;
		case decoder_module_generic:
		case decoder_module_amiga:
			success = source.Load(fileName, true);
			break;
		default:;
	}
	if (!success)
	{
		if (BASS_ErrorGetCode() == BASS_ERROR_NOTAVAIL)
			logror::Fatal("unalbe to determine length");
		else
			logror::Fatal("failed to load file (code: %1%)"), BASS_ErrorGetCode();
	}
	if (source.Channels() < 1 || source.Channels() > 2)
		logror::Fatal("usupported number of channels");

	std::cout << "type:" << TypeToString(source.BassChannelType()) << std::endl;
	std::cout << "length:" << source.Duration() << std::endl;
	std::cout << "bitrate:" << source.Bitrate() << std::endl;
	
	if (strcmp(argv[1], "--no-replay_gain"))
	{
		RG_SampleFormat format;
		format.sampleRate = source.Samplerate();
		format.sampleFormat = RG_SIGNED_16_BIT;
		format.numberChannels = source.Channels();
		format.interleaved = TRUE;
		RG_Context * context = RG_NewContext(&format);
		int16_t buffer[48000 * 8];
		static uint32_t const buffFrames = 
			bytes2frames<uint32_t, int16_t>(sizeof(buffer), source.Channels());
		uint32_t frames = 0;
		while ((frames = source.Process(buffer, buffFrames)) == buffFrames)	
			RG_Analyze(context, buffer, frames);
		std::cout << "replaygain:" << RG_GetTitleGain(context) << std::endl;
		RG_FreeContext(context);
	}
	return EXIT_SUCCESS;
}
