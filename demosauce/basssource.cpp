#include <boost/format.hpp>

#include "bass/bass.h"

#include "globals.h"
#include "basssource.h"

using namespace std;
using namespace boost;

HSTREAM streamSource = 0;
HMUSIC musicSource = 0;

void LogBassError()
{
	logg << "ERROR: bass error code: " << BASS_ErrorGetCode() << endl;	
}

bool SourceHasLength(DWORD source)
{
	const QWORD pos = BASS_ChannelGetLength(source, BASS_POS_BYTE);
	if (pos != static_cast<QWORD>(-1))
	{
		const WORD length = BASS_ChannelBytes2Seconds(source, pos);
		logg << format("INFO: time length %1% seconds\n") % length;
		return true;
	}
	else
	{
		logg << "ERROR: no time length for source\n";
	}
	return false;
}

DWORD BassSourceFillBuffer(DWORD source, void * buffer, DWORD length)
{
	const DWORD bytesRead = BASS_ChannelGetData(source, buffer, length);
	if (bytesRead != static_cast<DWORD>(-1))
	{
		if (BASS_ErrorGetCode() != BASS_ERROR_ENDED)
		{
			logg << "ERROR: failed to read from stream source\n";
			LogBassError();
		}
		return 0;
	}
	return bytesRead;
}

bool BassSourceLoadStream(string fileName)
{
	logg << "INFO: loading stream " << fileName << endl;
	// TODO make shure output is always in stereo and encoder's samplerate
	streamSource = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, BASS_STREAM_DECODE);
	if (streamSource == 0)
	{
		logg << "ERROR: failed to load stream source\n";
		LogBassError();
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
		logg << "WARNING: failed to free source\n";		
	streamSource = 0;
}

DWORD BassSourceFillBufferStream(void * buffer, DWORD length)
{
	return BassSourceFillBuffer(streamSource, buffer, length);
}

bool BassSourceLoadMusic(std::string fileName)
{
	logg << "INFO: loading module " << fileName << endl;
	// TODO make shure output is always in stereo
	DWORD const module_flags = BASS_MUSIC_DECODE | BASS_MUSIC_PRESCAN;
	musicSource = BASS_MusicLoad(FALSE, fileName.c_str(), 0, 0 , module_flags, setting::encoder_samplerate_int);
	if (musicSource== 0)
	{
		logg << "ERROR: failed to load music source\n";
		LogBassError();
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
		logg << "WARNING: failed to free source\n";
	musicSource = 0;
}

DWORD BassSourceFillBufferMusic(void * buffer, DWORD length)
{
	return BassSourceFillBuffer(musicSource, buffer, length);
}

void BassSourceInit()
{
	// nothing to init here atm. all had to be moved to basscast
}
