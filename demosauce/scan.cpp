#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

#include "bass/bass.h"

#include "replay_gain/replay_gain.h"

#include "logror.h"
#include "decoder_common.h"

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
	logror::LogSetConsoleLevel(logror::error);
	if (argc < 2 || (*argv[1] == '-' && argc < 3)) 
		logror::Fatal("not enough arguments");
	const char * fileName = argv[argc - 1];
	
	// open file
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	BASS_Init(0, 44100, 0, 0, NULL);
	const DecoderType decoderType = DecideDecoderType(fileName);
	DWORD channel = 0;
	switch(decoderType)
	{
		case decoder_codec_generic:
			channel = BASS_StreamCreateFile(FALSE, fileName, 0, 0, BASS_STREAM_DECODE | BASS_STREAM_PRESCAN);
			break;
		case decoder_module_generic:
		case decoder_module_amiga:
			channel = BASS_MusicLoad(FALSE, fileName, 0, 0, BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN, 44100);
			break;
		default:
			logror::Fatal("unsupported file type");
	}
	// conduct some tests
	BASS_CHANNELINFO info;
	BASS_ChannelGetInfo(channel, &info);
	
	if (info.chans < 1 || info.chans > 2)
		logror::Fatal("usupported number of channels");
	if (BASS_ErrorGetCode() != BASS_OK)
		logror::Fatal("BASS returned error code %1%"), BASS_ErrorGetCode();
	// get type
	std::cout << "type:" << TypeToString(info.ctype) << std::endl;
	// get length
	const QWORD chanLength = BASS_ChannelGetLength(channel, BASS_POS_BYTE);
	if (chanLength == static_cast<QWORD>(-1))
		logror::Fatal("unable to determine length (%1%)"), fileName;
	const int time = BASS_ChannelBytes2Seconds(channel, chanLength);
	std::cout << "length:" << time << std::endl;
	// get bitrate
	if (decoderType == decoder_codec_generic)
	{
		const DWORD fileLength = BASS_StreamGetFilePosition(channel, BASS_FILEPOS_END);
		const int bitrate = static_cast<int>(fileLength / (125 * time) + 0.5);
		std::cout << "bitrate:" << bitrate << std::endl;
	} 
	else
		std::cout << "bitrate:n/a\n";
	// get replay gain vaule
	if (strcmp(argv[1], "--no-replay_gain"))
	{
		RG_SampleFormat format;
		format.sampleRate = info.freq;
		format.sampleFormat = RG_SIGNED_16_BIT;
		format.numberChannels = info.chans;
		format.interleaved = TRUE;
		RG_Context * context = RG_NewContext(&format);
		char buffer[96000]; 
		DWORD bytes = 0;
		while ((bytes = BASS_ChannelGetData(channel, buffer, sizeof(buffer))) == sizeof(buffer))
		RG_Analyze(context, buffer, bytes / info.chans / RG_FormatSize(RG_SIGNED_16_BIT));
		std::cout << "replaygain:" << RG_GetTitleGain(context) << std::endl;
		RG_FreeContext(context);
	}
	// theoretically all resources should be freed here. just so you know :D
	return EXIT_SUCCESS;
}
