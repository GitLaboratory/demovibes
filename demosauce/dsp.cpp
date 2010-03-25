#include <cmath>
#include <cstring>
#include <vector>
#include <limits>
#include <algorithm>

#include<boost/static_assert.hpp>

#include "logror.h"
#include "dsp.h"

using namespace logror;

double DbToAmp(double db)
{
	return pow(10, db / 20);
}

double AmpToDb(double amp)
{
	return log10(amp) * 20;
}

//-----------------------------------------------------------------------------
typedef std::vector<Machine::MachinePtr> MachinePtrVector;

struct MachineStack::Pimpl
{
	MachinePtrVector machines;
};

MachineStack::MachineStack() : pimpl(new Pimpl) {}
MachineStack::~MachineStack() {} // needed or scoped_ptr may start whining

void MachineStack::Process(AudioStream& stream, uint32_t const frames)
{
	if (!source.get())
		return;
	return source->Process(stream, frames);
}

void MachineStack::AddMachine(MachinePtr& machine, size_t position)
{
	static size_t const maxMachines = 1000;
	if (machine.get() == this)
		return;
	MachinePtrVector & machines = pimpl->machines;
	if (position == add && machines.size() < maxMachines)
		machines.push_back(machine);
	else if (position <= maxMachines)
	{
		if (machines.size() < position + 1)
			machines.resize(position + 1);
		machines[position] = machine;
	}
}

void MachineStack::RemoveMachine(MachinePtr& machine)
{
	MachinePtrVector & machines = pimpl->machines;
	size_t lastMachine = 0;
	for (size_t i = 0; i < machines.size(); i++)
	{
		if (machines[i] == machine)
			machines[i] = MachinePtr(); // other way to do this?
		if (machines[i].get())
			lastMachine = i;
	}
	machines.resize(lastMachine + 1);
}

void MachineStack::UpdateRouting()
{
	MachinePtr sourceMachine;
	MachinePtrVector & machines = pimpl->machines;
	size_t i = 0;
	for(; i < machines.size() && !sourceMachine.get(); ++i)
		if (machines[i].get() && machines[i]->Enabled())
			sourceMachine = machines[i];
	for(; i < machines.size(); ++i)
		if (machines[i].get() && machines[i]->Enabled())
		{
			LogDebug("connect %1% -> %2%"), sourceMachine->Name(), machines[i]->Name();
			machines[i]->SetSource(sourceMachine);
			sourceMachine = machines[i];
		}
	SetSource(sourceMachine);
}

//-----------------------------------------------------------------------------
void MapChannels::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream, frames);
	uint32_t const inChannels = stream.Channels();
	stream.SetChannels(outChannels);

	if (inChannels == 1 && outChannels == 2)
		memcpy(stream.Buffer(1), stream.Buffer(0), stream.ChannelBytes());

	else if (inChannels == 2 && outChannels == 1)
	{
		float * left = stream.Buffer(0);
		float const * right = stream.Buffer(1);
		for (uint_fast32_t i = stream.Frames(); i; --i)
		{
			float const value = (*left + *right) * .5;
			*left++ = value;
			++right;
		}
	}
}

//-----------------------------------------------------------------------------
void LinearFade::Set(uint64_t startFrame, uint64_t endFrame, float beginAmp, float endAmp)
{
	if (startFrame >= endFrame || beginAmp < 0 || endAmp < 0)
		return;
	this->startFrame = startFrame;
	this->endFrame = endFrame;
	this->currentFrame = 0;
	this->amp = beginAmp;
	this->ampInc = (endAmp - beginAmp) / (endFrame - startFrame);
}

void LinearFade::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream, frames);
	uint32_t const readFrames = stream.Frames();
	uint32_t const endA = (startFrame < currentFrame) ? 0 :
		unsigned_min<uint32_t>(readFrames, startFrame - currentFrame);
	uint32_t const endB = (endFrame < currentFrame) ? 0 :
		unsigned_min<uint32_t>(readFrames, endFrame - currentFrame);
	currentFrame += readFrames;
	if (amp == 1 && (endA >= readFrames || endB == 0))
		return; // nothing to do; amp mignt not be exacly on target, so proximity check would be better

	for (uint_fast32_t iChan = 0; iChan < stream.Channels(); ++iChan)
	{
		float * out = stream.Buffer(iChan);
		uint_fast32_t i = 0;
		float a = amp;
		for (; i < endA; ++i)
			*out++ *= a;
		for (; i < endB; ++i)
			{ *out++ *= a; a += ampInc;	}
		for (; i < readFrames; ++i)
			*out++ *= a;
	}
	amp += ampInc * (endB - endA);
}

//-----------------------------------------------------------------------------
void Gain::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream, frames);
	for (uint_fast32_t iChan = 0; iChan < stream.Channels(); ++iChan)
	{
		float* out = stream.Buffer(iChan);
		for (uint_fast32_t i = stream.Frames(); i; --i)
			*out++ *= amp;
	}
}

//-----------------------------------------------------------------------------
void NoiseSource::SetDuration(uint64_t duration)
{
	this->duration = duration;
	currentFrame = 0;
}

void NoiseSource::Process(AudioStream& stream, uint32_t const frames)
{
	if (currentFrame >= duration)
	{
		stream.SetFrames(0);
		stream.endOfStream = true;
		return;
	}
	float const gah = 1.0 / RAND_MAX;
	const uint_fast32_t procFrames = unsigned_min<uint32_t>(duration - currentFrame, frames);

	for (uint_fast32_t iChan = 0; iChan < stream.Channels(); ++iChan)
	{
		float* out = stream.Buffer(iChan);
		for (uint_fast32_t i = procFrames; i; --i)
			*out++ = gah * rand();
	}

	currentFrame += procFrames;
	stream.endOfStream = currentFrame >= duration;
	stream.SetFrames(procFrames);
}

//-----------------------------------------------------------------------------
void MixChannels::Set(float llAmp, float lrAmp, float rrAmp, float rlAmp)
{
	this->llAmp = llAmp;
	this->lrAmp = lrAmp;
	this->rrAmp = rrAmp;
	this->rlAmp = rlAmp;
}

void MixChannels::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream, frames);
	if (stream.Channels() != 2)
		return;
	float* left = stream.Buffer(0);
	float* right = stream.Buffer(1);
	for (uint_fast32_t i = stream.Frames(); i; --i)
	{
		float const newLeft = llAmp * *left + lrAmp * *right;
		float const newRight = rrAmp * *right + rlAmp * *left;
		*left++ = newLeft;
		*right++ = newRight;
	}
}

//-----------------------------------------------------------------------------
// ok, this is supposed to be a brickwall limiter, but I have absolutely no
// indea about the theory behind one. so I just made some stuff up!
Brickwall::Brickwall() :
	peak(.999),
	gain(.999),
	gainInc(0),
	attackLength(441), // 1 ms at 44100
	releaseLength(441),
	streamPos(0),
	attackEnd(0),
	sustainEnd(0),
	releaseEnd(0)
{}

void Brickwall::Set(uint32_t attackFrames, uint32_t releaseFrames)
{
	attackLength = attackFrames;
	releaseLength = releaseFrames;
}

void Brickwall::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream1, frames);
	assert(stream1.Channels() > 0);
	uint32_t procFrames0 = std::min(stream0.Frames(), frames);
	uint32_t procFrames1 = stream1.Frames() < procFrames0 ? 0 :
		stream1.Frames() - procFrames0;
	uint32_t ampFrames = procFrames0 + stream1.Frames();

	if (ampBuffer.Size() < ampFrames)
		ampBuffer.Resize(ampFrames);
	if (mixBuffer.Size() < stream1.Frames())
		mixBuffer.Resize(stream1.Frames());

	// prepare data for peak scanning
	{
		assert(stream1.Channels() <= 2);
		float* buff = mixBuffer.Get();
		float* in0 = stream1.Buffer(0);
		if (stream1.Channels() == 1)
			for (uint_fast32_t i = stream1.Frames(); i; --i)
				*buff++ = fabs(*in0++);
		else
		{
			float* in1 = stream1.Buffer(1);
			for (uint_fast32_t i = stream1.Frames(); i; --i)
				*buff++ = std::max(fabs(*in0++), fabs(*in1++));
		}
	}

	// scan data at beginning of stream
	if (streamPos < attackLength)
	{
		float* in = mixBuffer.Get();
		uint64_t i = streamPos;
		for (; i < std::min(stream1.Frames(), attackLength); ++i, ++in)
			if (*in > peak)
				peak = fabs(*in);
		if (i == attackLength)
			procFrames1 = procFrames1 - std::min(procFrames1, attackLength);
		else if (!stream1.endOfStream)
		{   // not enouth frames for regular processing
			streamPos = i;
			stream.SetFrames(0);
			stream0.Append(stream1);
			return;
		}
		gain = .999 / peak; // peak always >= 1
	}

	{
		float* amp = ampBuffer.Get();
		float* in = mixBuffer.Get();

		for (uint_fast32_t i = stream1.Frames(); i; --i, ++streamPos)
		{
			float const value = *in++;
			if (value > peak)
			{
				peak = value;
				gainInc = (.999 / peak - gain) / attackLength;
				attackEnd = streamPos + attackLength;
			}

			if (value > .999)
			{
				sustainEnd = streamPos + attackLength;
				releaseEnd = 0;
			}

			if (streamPos == attackEnd)
				gainInc = 0;

			if (streamPos == sustainEnd)
			{
				gainInc = (.999 - gain) / releaseLength;
				releaseEnd = streamPos + releaseLength;
				peak = .999;
			}

			if (streamPos == releaseEnd)
			{
				gain = .999;
				gainInc = 0;
			}

			*amp++ = gain;
			gain += gainInc;
		}
	}

	// may happen at end of stream
	if (stream1.Frames() < ampFrames)
	{
		float* amp = ampBuffer.Get();
		for (uint_fast32_t i = ampFrames - stream1.Frames(); i; --i, ++streamPos)
			*amp++ = gain;
	}

	stream.SetChannels(stream1.Channels());
	if (stream.MaxFrames() < procFrames0 + procFrames1)
		stream.Resize(procFrames0 + procFrames1);
	stream.SetFrames(procFrames0 + procFrames1);

	for (uint32_t iChan = 0; iChan < stream1.Channels(); ++iChan)
	{
		float* amp = ampBuffer.Get();
		float* in = stream0.Buffer(iChan);
		float* out = stream.Buffer(iChan);
		for (uint_fast32_t i = procFrames0; i; --i)
			*out++ = *in++ * *amp++;
		in = stream1.Buffer(iChan);
		for (uint_fast32_t i = procFrames1; i; --i)
			*out++ = *in++ * *amp++;
	}

// uncomment to visualize applied gain in left channel
//	{
//		float* amp = ampBuffer.Get();
//		float* in = stream0.Buffer(0);
//		float* out = stream.Buffer(0);
//		for (uint_fast32_t i = procFrames0; i; --i)
//			*out++ = *amp++;
//		in = stream1.Buffer(0);
//		for (uint_fast32_t i = procFrames1; i; --i)
//			*out++ = *amp++;
//	}
	stream0.Drop(procFrames0);
	stream1.Drop(procFrames1);
	stream0.Append(stream1);

	stream.endOfStream = false;
	if (stream1.endOfStream && procFrames0 + stream1.Frames() <= frames)
	{
		gain = .999;
		peak = .999;
		gainInc = 0;
		stream.endOfStream = true;
	}
	if(stream.endOfStream) LogDebug("eos brick %1% frames left"), stream.Frames();
}

//-----------------------------------------------------------------------------
void Peaky::Process(AudioStream& stream, uint32_t const frames)
{
	source->Process(stream, frames);
	float max = peak;
	for (uint_fast32_t iChan = 0; iChan < stream.Channels(); ++iChan)
	{
		float const * in = stream.Buffer(iChan);
		for (uint_fast32_t i = stream.Frames(); i; --i)
		{
			float const value = fabs(*in++);
			if (value > max)
				max = value;
		}
	}
	peak = max;
}

