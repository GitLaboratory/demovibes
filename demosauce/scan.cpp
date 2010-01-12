#include <cstdlib>
#include <string>
#include <iostream>

#include "bass/bass.h"

#include "replay_gain/replay_gain.h"

#include "logror.h"
#include "decoder_common.h"

int main(int argc, char* argv[])
{
	const char * fileName = argv[1];
	logror::LogSetConsoleLevel(logror::error);
	if (argc < 2)
		logror::Fatal("not enough arguments");
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
	BASS_CHANNELINFO info;
	BASS_ChannelGetInfo(channel, &info);
	if (info.chans < 1 || info.chans > 2)
		logror::Fatal("usupported number of channels");
	if (BASS_ErrorGetCode() != BASS_OK)
		logror::Fatal("BASS returned error code %1%"), BASS_ErrorGetCode();
	const QWORD chanLength = BASS_ChannelGetLength(channel, BASS_POS_BYTE);
	if (chanLength == static_cast<QWORD>(-1))
		logror::Fatal("unable to determine length (%1%)"), fileName;
	const int time = BASS_ChannelBytes2Seconds(channel, chanLength);
	std::cout << "length:" << time << std::endl;
	if (decoderType == decoder_codec_generic)
	{
		const DWORD fileLength = BASS_StreamGetFilePosition(channel, BASS_FILEPOS_END);
		const int bitrate = static_cast<int>(fileLength / (125 * time) + 0.5);
		std::cout << "bitrate:" << bitrate << std::endl;
	} 
	else
		std::cout << "bitrate:n/a\n";
	
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
	// theoretically all resources should be freed here. just so you know :D
	return EXIT_SUCCESS;
}
