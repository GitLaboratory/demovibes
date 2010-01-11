#include <boost/numeric/conversion/cast.hpp>

#include "bass/bass.h"

#include "globals.h"
#include "basssource.h"

using namespace std;
using namespace boost;
using namespace logror;

HSTREAM streamSource = 0;
HMUSIC musicSource = 0;

bool SourceHasLength(DWORD source)
{
	const QWORD pos = BASS_ChannelGetLength(source, BASS_POS_BYTE);
	if (pos != static_cast<QWORD>(-1))
	{
		const WORD length = BASS_ChannelBytes2Seconds(source, pos);
		Log(info, "length %1% seconds"), length;
		return true;
	}
	else
	{
		Error("no time length for source");
	}
	return false;
}

uint32_t BassSourceFillBuffer(DWORD source, void * buffer, uint32_t length)
{
	const DWORD bytesRead = BASS_ChannelGetData(source, buffer, length);
	if (bytesRead == static_cast<DWORD>(-1))
	{
		if (BASS_ErrorGetCode() != BASS_ERROR_ENDED)
			Error("failed to read from source (%1%)"), BASS_ErrorGetCode();
		return 0;
	}
	return numeric_cast<uint32_t>(bytesRead);
}

bool BassSourceLoadStream(string fileName)
{
	Log(info, "loading stream %1%"), fileName;
	// TODO make shure output is always in stereo and encoder's samplerate
	streamSource = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
	if (streamSource == 0)
	{
		Error("failed to load stream source (%1%)"), BASS_ErrorGetCode();
		return false;
	}
	if (!SourceHasLength(streamSource))
	{
		BassSourceFreeStream();
		return false;
	}
	return true;
}

void BassSourceFreeStream()
{
	if (!BASS_StreamFree(streamSource))
		Log(warning, "failed to free stream source");
	streamSource = 0;
}

uint32_t BassSourceFillBufferStream(void * buffer, uint32_t length)
{
	return BassSourceFillBuffer(streamSource, buffer, length);
}

bool BassSourceLoadMusic(std::string fileName)
{
	Log(info, "loading module %1%"), fileName;
	// TODO make shure output is always in stereo
	DWORD const module_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN;
	musicSource = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , module_flags, setting::encoder_samplerate);
	if (musicSource== 0)
	{
		Error("failed to load music source (%1%)"), BASS_ErrorGetCode();
		return false;
	}
	if (!SourceHasLength(musicSource))
	{
		BassSourceFreeMusic();
		return false;
	}
	return true;
}

void BassSourceFreeMusic()
{
	if (!BASS_MusicFree(musicSource))
		Log(warning, "failed to free music source");
	musicSource = 0;
}

uint32_t BassSourceFillBufferMusic(void * buffer, uint32_t length)
{
	return BassSourceFillBuffer(musicSource, buffer, length);
}

void BassSourceInit()
{
	// nothing to init here atm. all had to be moved to basscast
}
