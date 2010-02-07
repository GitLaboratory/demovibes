#include <cstring>
#include <vector>

#include "libsamplerate/samplerate.h"

#include "logror.h"

#include "convert.h"

using namespace std;
using namespace logror;

struct Resample::Pimpl
{
	void Free();
	void UpdateChannels(uint32_t channels);
	void Reset();
	double ratio;
	AlignedBuffer<float> inBuffer;
	vector<SRC_STATE*> states;
	Pimpl() : ratio(1) {}
};

Resample::Resample() : pimpl(new Pimpl) {}

Resample::~Resample()
{
	pimpl->Free();
}

void Resample::Pimpl::UpdateChannels(uint32_t channels)
{	
	if (states.size() > channels)
		for (size_t i = channels; i < states.size(); ++i)
			src_delete(states[i]);

	if (states.size() < channels)
	{
		for (size_t i = states.size(); i < channels; ++i)
		{
			LogDebug("new resampler");
			int err = 0;
			SRC_STATE* state = src_new(SRC_SINC_FASTEST, 1, &err);
			if (err)
				Log(warning, "src_new error: %1%"), src_strerror(err);
			else
				states.push_back(state);
		}
		Reset();
	}
}

uint32_t Resample::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
		
	if (pimpl->ratio == 1)
		return source->Process(buffer, frames);
	pimpl->UpdateChannels(source->Channels());
	
	uint32_t const framesToRead = frames / pimpl->ratio;
	
	if (pimpl->inBuffer.Size() < framesToRead * source->Channels())
		pimpl->inBuffer.Resize(framesToRead * source->Channels());
	
	uint32_t const inFrames = source->Process(pimpl->inBuffer.Get(), framesToRead);
	bool const eos = inFrames != framesToRead;
	uint32_t outFrames = 0;
	
	for (size_t i = 0; i < pimpl->states.size(); ++i)
	{
		SRC_DATA data;
		data.src_ratio = pimpl->ratio;
   		data.data_in = pimpl->inBuffer.Get() + i * inFrames;
   		data.data_out = buffer + i * frames;
		data.input_frames = inFrames;
		data.output_frames = frames;
		data.end_of_input = eos ? 1 : 0;
				
		SRC_STATE* state = pimpl->states[i];
	
		int const err = src_process(state, &data);
		
//		LogDebug("%1% %2% %3% %4%"), i, framesToRead, inFrames, frames;
//		LogDebug("%1% %2%"), data.input_frames_used, data.output_frames_gen;
		if (err)
		{
			Error("src_process error: %1%"), src_strerror(err);
			return 0;
		}
		outFrames = data.output_frames_gen;
		if (outFrames < frames && !eos) 
		{
			// happens on first processing block...
			// align data to buffer end and zero the front
			size_t outBytes = FramesInBytes<float>(outFrames, 1);
			uint32_t gapFrames = frames - outFrames;
			memmove(data.data_out + gapFrames, data.data_out, outBytes);
			memset(data.data_out, 0, FramesInBytes<float>(gapFrames, 1));
		}
	}	
	return eos ? outFrames : frames;
}

void Resample::Pimpl::Reset()
{
	for (size_t i = 0; i < states.size(); ++i)
	{
		int err = src_reset(states[i]);
		if (err)
			Log(warning, "src_reset error: %1%"), src_strerror(err);
	}
}

void Resample::Set(uint32_t sourceRate, uint32_t outRate)
{
	pimpl->ratio = static_cast<double>(outRate) / sourceRate;
	pimpl->Reset();
}

void Resample::Pimpl::Free()
{
	for (size_t i = 0; i < states.size(); ++i)
		src_delete(states[i]);
	states.clear();
}
