#ifndef _H_CONVERT_
#define _H_CONVERT_

#include <cassert>
#include <limits>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include "dsp.h"

//-----------------------------------------------------------------------------

class Resample : public Machine
{
public:
	Resample();
	virtual ~Resample();
	// overwriting
	void Process(AudioStream & stream, uint32_t const frames);
	std::string Name() const { return "Resample"; }
	
	void Set(uint32_t sourceRate, uint32_t outRate);
private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

//-----------------------------------------------------------------------------

template <typename SampleType>
class ConvertFromInterleaved : boost::noncopyable
{
public:

	ConvertFromInterleaved() : channels(0) {}
		
	SampleType* Buffer(uint32_t frames, uint32_t channels)
	{
		this->channels = channels;
		if (buffer.Size() < frames * channels)
			buffer.Resize(frames * channels);
		return buffer.Get(); 
	}
	
	void Process(AudioStream & stream, uint32_t const frames);
	
private:
	uint32_t channels;
	AlignedBuffer<SampleType> buffer;
};

template<> inline void ConvertFromInterleaved<int16_t>
::Process(AudioStream & stream, uint32_t const frames)
{
	assert(buffer.Size() >= frames * channels);
	
	stream.SetChannels(channels);
    if (stream.MaxFrames() < frames)
        stream.Resize(frames);
    stream.SetFrames(frames);
	
	static float const range = 1.0 / -std::numeric_limits<int16_t>::min(); //2's complement
	for (uint_fast32_t iChan = 0; iChan < channels; ++iChan)
	{	
		int16_t const * in = buffer.Get() + iChan;
		float* out = stream.Buffer(iChan);
		for (uint_fast32_t i = frames; i; --i)
		{ 
			*out++ = range * *in; 
			in += channels; 
		}
	}
}

template<> inline void ConvertFromInterleaved<float>
::Process(AudioStream & stream, uint32_t const frames)
{
	assert(buffer.Size() >= frames * channels);
	
	stream.SetChannels(channels);
    if (stream.MaxFrames() < frames)
        stream.Resize(frames);
    stream.SetFrames(frames);

	for (uint_fast32_t iChan = 0; iChan < channels; ++iChan)
	{	
		float const * in = buffer.Get() + iChan;
		float* out = stream.Buffer(iChan);
		for (uint_fast32_t i = frames; i; --i)
		{ 
			*out++ = *in; 
			in += channels; 
		}
	}
}

//-----------------------------------------------------------------------------

class ConvertToInterleaved
{
public:
	
	template <typename MachineType> 
	void SetSource(MachineType & machine) 
	{ 
		source = boost::static_pointer_cast<Machine>(machine); 
	}
	
	template <typename SampleType>
	void Process(SampleType* const outSamples, uint32_t const frames);

	AudioStream const & SourceStream() const { return inStream; }
	
private:
	Machine::MachinePtr source;
	AudioStream inStream;
};

template <> inline void ConvertToInterleaved
::Process(int16_t* const outSamples, uint32_t const frames)
{
 	static int16_t const range = std::numeric_limits<int16_t>::max();
	source->Process(inStream, frames);
	
	uint32_t const channels = inStream.Channels();
	for (uint_fast32_t iChan = 0; iChan < channels; ++iChan)
	{
		float* in = inStream.Buffer(iChan);
		int16_t* out = outSamples + iChan;
		for (uint_fast32_t i = frames; i; --i)
		{ 
			*out = static_cast<int16_t>(range * *in++); 
			out += channels; 
		}
	}
}

#endif
