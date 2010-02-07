// mpeg4 segfaults. still trying to find out why...

#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "logror.h"
#include "convert.h"
#include "avsource.h"

extern "C" 
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>

using namespace std;
using boost::numeric_cast;
using namespace logror;

typedef int16_t sample_t;

struct AvSource::Pimpl
{
	void Free();
	uint32_t Process(float * const buffer, uint32_t const frames);
	
	ConvertFromInterleaved<sample_t> converter;
	string fileName;
	
	AVFormatContext* formatContext;
	AVCodecContext* codecContext;
	AVCodec* codec;
	int audioStreamIndex;
	
	int bufferPos;
	Pimpl() : 
		formatContext(0), 
		codecContext(0), 
		codec(0), 
		audioStreamIndex(-1), 
		bufferPos(0) {}
};

AvSource::AvSource():
	pimpl(new Pimpl)
{
	av_register_all();
	pimpl->Free();
}

bool AvSource::Load(string fileName, bool prescan)
{
	pimpl->Free();
	pimpl->fileName = fileName;
	
	Log(info, "avsource loading %1%"), fileName;
	if (av_open_input_file(&pimpl->formatContext, fileName.c_str(), NULL, 0, NULL) != 0)
	{ 
		Log(warning, "can't open file %1%"), fileName; 
		return false; 
	}
	
	if (av_find_stream_info(pimpl->formatContext) < 0)
	{
		Log(warning, "no stream information %1%"), fileName; 
		return false; 
	}
	
	for (unsigned int i = 0; i < pimpl->formatContext->nb_streams; i++) // find first stream
		if (pimpl->formatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
		{
			pimpl->audioStreamIndex = i;
			break;
		}
			
	if (pimpl->audioStreamIndex == -1)
	{ 
		Log(warning, "no audio stream :( %1%"), fileName; 
		return false; 
	}
		
	pimpl->codecContext = pimpl->formatContext->streams[pimpl->audioStreamIndex]->codec;
	
	pimpl->codec = avcodec_find_decoder(pimpl->codecContext->codec_id);
	if (!pimpl->codec)
	{ 
		Log(warning, "unsupported codec %1%"), fileName; 
		return false;
	}
		
	if (avcodec_open(pimpl->codecContext, pimpl->codec) < 0)
	{ 
		Log(warning, "failed to open codec %1%"), fileName; 
		return false; 
	}		
	
	return true;
}

static uint32_t const minFrames = 2 * BytesInFrames<uint32_t, char>(AVCODEC_MAX_AUDIO_FRAME_SIZE, 1);

uint32_t AvSource::Pimpl::Process(float * const buffer, uint32_t const frames)
{
	return frames;
}

uint32_t AvSource::Process(float * const buffer, uint32_t const frames)
{
	return pimpl->Process(buffer, frames);
}

void AvSource::Pimpl::Free()
{
	if (codecContext)
		avcodec_close(codecContext);
	if (formatContext)
		av_close_input_file(formatContext);
	codec = 0;
	codecContext = 0;
	formatContext = 0;
	bufferPos = 0;
}

AvSource::~AvSource()
{
	pimpl->Free();
}

uint32_t AvSource::AVCodecType() const
{
	return static_cast<uint32_t>(pimpl->codec->id);
}

uint32_t AvSource::Channels() const
{
	return numeric_cast<uint32_t>(pimpl->codecContext->channels);
}
	
uint32_t AvSource::AvSource::Samplerate() const
{
	return numeric_cast<uint32_t>(pimpl->codecContext->sample_rate);
}

uint32_t AvSource::Bitrate() const
{ 
	return numeric_cast<uint32_t>(pimpl->codecContext->bit_rate) / 1000;
}  

double AvSource::Duration() const
{
	return numeric_cast<double>(pimpl->formatContext->duration) / AV_TIME_BASE;
}

bool AvSource::CheckExtension(string const fileName)
{
	boost::filesystem::path file(fileName);
	string name = file.filename();
	
	// anything ffmpeg supports can be added here, even .ra, haha. but for now we stick to most common stuff
	static size_t const elements = 10;
	char const * ext[elements] = {".mp3", ".ogg", ".m4a", ".wma", ".flac", ".acc", ".mp4", ".mp2", ".mp1", ".wav"};
	for (size_t i = 0; i < elements; ++i)
		if (boost::iends_with(name, ext[i]))
			return true;
	return false;
}

/*

	if (bufferPos < 0)
	{ 
		Error("you'll rue this day, on which you tried to read beyond the end of a file. now START RUEING!"); 
		return 0; 
	}

	uint32_t const channels = numeric_cast<uint32_t>(codecContext->channels);
	if (channels < 1)
		return 0;

	size_t const cBuffferSize = max(minFrames, frames * 2);
	int8_t* cBuffer = reinterpret_cast<int8_t*>(converter.Buffer(cBuffferSize, channels));
	int const targetBytes = FramesInBytes<sample_t>(frames, channels);

	AVPacket packet = {0};
	bool endOfStream = false;
	
	while (bufferPos < targetBytes && !endOfStream)
	{
		if (av_read_frame(formatContext, &packet) != 0) // demux/read packet
			break; // end of stream
			
		int packetSize = packet.size;
		uint8_t* packetData = packet.data;
		
		if  (packet.stream_index == audioStreamIndex)
		{
			int16_t* tBuffer = reinterpret_cast<int16_t*>(cBuffer + bufferPos);
			int decodedSize = 0;
			int length = 0;
		
			while (packetSize > 0)
			{
				decodedSize = targetBytes;
				memset(tBuffer, 0, targetBytes);
				length = avcodec_decode_audio2(codecContext, tBuffer, &decodedSize, packetData, packetSize);
		
				packetSize -= length;
				packetData += length;
				bufferPos += decodedSize;
			}
			endOfStream = length < 1;
		}
	}
	
	if (packet.data)
		av_free_packet(&packet);
	
	size_t const usedBytes = min(targetBytes, bufferPos);	
	uint32_t const usedFrames = BytesInFrames<uint32_t, sample_t>(usedBytes, channels);
	converter.Process(buffer, usedFrames);
	
	if (bufferPos > targetBytes)
		memmove(cBuffer, cBuffer + targetBytes, bufferPos - targetBytes);
	bufferPos -= targetBytes;
	return usedFrames;
*/
