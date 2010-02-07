#ifndef _H_CONVERT_
#define _H_CONVERT_

#include <limits>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include "misc.h"
#include "dsp.h"

//-----------------------------------------------------------------------------

class Resample : public Machine
{
public:
	Resample();
	virtual ~Resample();
	// overwriting
	uint32_t Process(float * const buffer, uint32_t const frames);
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
	
	void Process(float * const outSamples, uint32_t const frames);
	
private:
	uint32_t channels;
	AlignedBuffer<SampleType> buffer;
};

template<> inline void ConvertFromInterleaved<int16_t>
::Process(float * const outSamples, uint32_t const frames)
{
	if (buffer.Size() < frames * channels)
		return;
	float const range = 1.0 / -std::numeric_limits<int16_t>::min(); //2's complement
	uint32_t const chan = channels;
	float* out = outSamples;
	for (uint_fast32_t iChannel = 0; iChannel < chan; ++iChannel)
	{	
		int16_t const * in = buffer.Get() + iChannel;
		for (uint_fast32_t i = 0; i < frames; ++i)
			{ *out++ = range * *in; in += chan; }
	}
}

template<> inline void ConvertFromInterleaved<float>
::Process(float * const outSamples, uint32_t const frames)
{
	if (buffer.Size() < frames * channels)
		return;
	uint32_t const chan = channels;
	float* out = outSamples;
	for (uint_fast32_t iChannel = 0; iChannel < channels; ++iChannel)
	{	
		float const * in = buffer.Get() + iChannel;
		for (uint_fast32_t i = 0; i < frames; ++i)
			{ *out++ = *in; in += chan; }
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
	uint32_t Process(SampleType* const outSamples, uint32_t const frames);

private:

	Machine::MachinePtr source;
	AlignedBuffer<float> buffer;
};

template <> inline uint32_t ConvertToInterleaved
::Process(int16_t* const outSamples, uint32_t const frames)
{
	if (!source.get())
		return 0;
 	int16_t const range = std::numeric_limits<int16_t>::max();
	uint32_t const chan = source->Channels();
	if (buffer.Size() < frames * chan)
		buffer.Resize(frames * chan);
	float * in = buffer.Get();
	uint32_t const readFrames = source->Process(in, frames);
	for (uint_fast32_t iChannel = 0; iChannel < chan; ++iChannel)
	{
		int16_t* out = outSamples + iChannel;
		for (uint_fast32_t i = 0; i < frames; ++i)
			{ *out = static_cast<int16_t>(range * *in++); out += chan; }
	}
	return readFrames;
}

#endif
