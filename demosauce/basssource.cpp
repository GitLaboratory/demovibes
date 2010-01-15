#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"
#include "bass/bass_aac.h"
#include "bass/bassflac.h"

#include "globals.h"
#include "decoder_common.h"
#include "basssource.h"

using namespace std;
using namespace boost;
using namespace logror;

struct BassSource::Pimpl
{
	void Free();
	DWORD channel;
	bool isModule;
};

BassSource::BassSource():
	pimpl(new Pimpl)
{
	pimpl->channel = 0;
	pimpl->isModule = false;
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	if (!BASS_Init(0, setting::encoder_samplerate, 0, 0, NULL) && 
		BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
		Fatal("BASS init failed (%1%)"), BASS_ErrorGetCode();
}

BassSource::~BassSource()
{
	pimpl->Free();
}

bool BassSource::Load(std::string fileName)
{
	pimpl->Free();
	DWORD & channel = pimpl->channel; 
	Log(info, "loading %1%"), fileName;
	switch (DecideDecoderType(fileName))
	{
		case decoder_codec_generic:
			channel = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
			break;
		case decoder_codec_aac:
			channel = BASS_AAC_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
			break;
		case decoder_codec_mp4:
			channel = BASS_MP4_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
			break;
		case decoder_codec_flac:
			channel = BASS_FLAC_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
			break;
		case decoder_module_generic:
			pimpl->isModule = true;
			static const DWORD generic_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN | BASS_MUSIC_RAMP;
			channel = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , generic_flags, setting::encoder_samplerate);
			break;
		case decoder_module_amiga:
			pimpl->isModule = true;
			static const DWORD amiga_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN | BASS_MUSIC_NONINTER | BASS_MUSIC_PT1MOD;
			channel = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , amiga_flags, setting::encoder_samplerate);
			break;
		default:;
	}
	if (channel == 0)
	{
		Error("failed to load %1% (%2%)"), fileName, BASS_ErrorGetCode();
		return false;
	}	
	const QWORD length = BASS_ChannelGetLength(channel, BASS_POS_BYTE);
	if (length != static_cast<QWORD>(-1))
	{
		const float duration = BASS_ChannelBytes2Seconds(channel, length);
		Log(info, "duration %1% seconds"), duration;
	} 
	else
	{
		Error("failed to determine duration - %1%"), fileName;
		pimpl->Free();
		return false;
	}
	return true;	
}

void 
BassSource::Pimpl::Free()
{
	if (channel != 0)
	{
		if (isModule)
			BASS_MusicFree(channel);
		else
			BASS_StreamFree(channel);
		if (BASS_ErrorGetCode() != BASS_OK);
			Log(warning, "failed to free BASS channel (%1%)"), BASS_ErrorGetCode();
		channel = 0;
	}
	isModule = false;
}

uint32_t 
BassSource::FillBuffer(void * buffer, uint32_t length)
{
	const DWORD bytesRead = BASS_ChannelGetData(pimpl->channel, buffer, length);
	if (bytesRead == static_cast<DWORD>(-1))
	{
		if (BASS_ErrorGetCode() != BASS_ERROR_ENDED)
			Error("failed to read from channel (%1%)"), BASS_ErrorGetCode();
		return 0;
	}
	return numeric_cast<uint32_t>(bytesRead);
}
