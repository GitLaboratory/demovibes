#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"
#include "bass/bass_aac.h"
#include "bass/bassflac.h"

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
	uint32_t samplerate;
	float duration;
	uint64_t currentFrame;
	uint64_t lastFrame;
	BASS_CHANNELINFO channelInfo;
};

BassSource::BassSource():
	pimpl(new Pimpl)
{
	pimpl->channel = 0;
	pimpl->isModule = false;
	pimpl->samplerate = 44100;
	pimpl->duration = 0;
	pimpl->currentFrame = 0;
	pimpl->lastFrame = static_cast<uint64_t>(-1);
	//memset(&pimpl->channelInfo, 0, sizeof(BASS_CHANNELINFO));
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	if (!BASS_Init(0, 44100, 0, 0, NULL) && 
		BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
		Fatal("BASS init failed (%1%)"), BASS_ErrorGetCode();
}

BassSource::~BassSource()
{
	pimpl->Free();
}

bool BassSource::Load(std::string fileName, bool prescan)
{
	pimpl->Free();
	DWORD & channel = pimpl->channel; 
	Log(info, "loading %1%"), fileName;
	DWORD const stream_flags = BASS_STREAM_DECODE | (prescan ? BASS_STREAM_PRESCAN : 0);
	DWORD const generic_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN | BASS_MUSIC_RAMP;
	DWORD const amiga_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN | BASS_MUSIC_NONINTER | BASS_MUSIC_PT1MOD;
	switch (DecideDecoderType(fileName))
	{
		case decoder_codec_generic:
			channel = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, stream_flags);
			break;
		case decoder_codec_aac:
			channel = BASS_AAC_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, stream_flags);
			break;
		case decoder_codec_mp4:
			channel = BASS_MP4_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, stream_flags);
			break;
		case decoder_codec_flac:
			channel = BASS_FLAC_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, stream_flags);
			break;
		case decoder_module_generic:
			pimpl->isModule = true;
			channel = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , generic_flags, pimpl->samplerate);
			break;
		case decoder_module_amiga:
			pimpl->isModule = true;
			channel = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , amiga_flags, pimpl->samplerate);
			break;
		default:;
	}
	if (channel == 0)
	{
		Error("failed to load %1% (%2%)"), fileName, BASS_ErrorGetCode();
		return false;
	}
	BASS_ChannelGetInfo(channel, &pimpl->channelInfo);
	const QWORD length = BASS_ChannelGetLength(channel, BASS_POS_BYTE);
	pimpl->duration = BASS_ChannelBytes2Seconds(channel, length);
	if (length != static_cast<QWORD>(-1))
		Log(info, "duration %1% seconds"), pimpl->duration;
	else
	{
		Error("failed to determine duration of %1%"), fileName;
		pimpl->Free();
		return false;
	}
	return true;	
}

void BassSource::Pimpl::Free()
{
	if (channel != 0)
	{
		if (isModule)
			BASS_MusicFree(channel);
		else
			BASS_StreamFree(channel);
		if (BASS_ErrorGetCode() != BASS_OK)
			Log(warning, "failed to free BASS channel (%1%)"), BASS_ErrorGetCode();
		channel = 0;
	}
	isModule = false;
	duration = 0;
	currentFrame = 0;
	memset(&channelInfo, 0, sizeof(BASS_CHANNELINFO));
}

uint32_t BassSource::Process(int16_t * const buffer, uint32_t const frames)
{
	uint32_t const channels = Channels();
	uint32_t const framesToRead = 
		unsigned_min<uint32_t>(frames, pimpl->lastFrame - pimpl->currentFrame);
	DWORD const length = frames2bytes<DWORD, int16_t>(framesToRead, channels);
	if (buffer == NULL || length == 0)
		return 0;
	DWORD const bytesRead = BASS_ChannelGetData(pimpl->channel, buffer, length);
	if (bytesRead == static_cast<DWORD>(-1))
	{
		if (BASS_ErrorGetCode() != BASS_ERROR_ENDED)
			Error("failed to read from channel (%1%)"), BASS_ErrorGetCode();
		return 0;
	}
	uint32_t const framesRead = bytes2frames<uint32_t, int16_t>(bytesRead, channels);
	pimpl->currentFrame += framesRead;
	return framesRead;
}

void BassSource::SetSamplerate(uint32_t moduleSamplerate)
{ pimpl->samplerate = moduleSamplerate; }

void BassSource::SetLoopvogel(float duration)
{ 
	pimpl->duration = duration;
	pimpl->lastFrame = numeric_cast<uint64_t>(duration * Samplerate());
	// enable looping here
}

uint32_t BassSource::BassChannelType() const
{ return static_cast<uint32_t>(pimpl->channelInfo.ctype); }

uint32_t BassSource::Channels() const
{ return static_cast<uint32_t>(pimpl->channelInfo.chans); }

uint32_t BassSource::Samplerate() const
{ return static_cast<uint32_t>(pimpl->channelInfo.freq); }

uint32_t BassSource::Bitrate() const
{
	DWORD const fileLength = BASS_StreamGetFilePosition(pimpl->channel, BASS_FILEPOS_END);
 	uint32_t const bitrate = static_cast<uint32_t>(fileLength / (125 * Duration()) + 0.5);
	return pimpl->isModule ? 0 : bitrate;
}

float BassSource::Duration() const { return pimpl->duration; }
