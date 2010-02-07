#include <cmath>
#include <cstring>
#include <vector>
#include <limits>
#include <algorithm>

#include<boost/static_assert.hpp>

#include "logror.h"
#include "dsp.h"

using namespace logror;

double DbToAmp(double db) { return pow(10, db / 20); }
double AmpToDb(double amp) { return log10(amp) * 20; }

//-----------------------------------------------------------------------------
typedef std::vector<Machine::MachinePtr> MachinePtrVector;

struct MachineStack::Pimpl
{
	MachinePtrVector machines;
};

MachineStack::MachineStack() : pimpl(new Pimpl) {}
MachineStack::~MachineStack() {} // needed or scoped_ptr may start whining
	
uint32_t MachineStack::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	return source->Process(buffer, frames);
}

void MachineStack::AddMachine(MachinePtr & machine, size_t position)
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

void MachineStack::RemoveMachine(MachinePtr & machine)
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
		if (machines[i].get() && !machines[i]->Bypass())
			sourceMachine = machines[i];
	for(; i < machines.size(); ++i)
		if (machines[i].get() && !machines[i]->Bypass())
		{
			LogDebug("connect %1% -> %2%"), sourceMachine->Name(), machines[i]->Name();
			machines[i]->SetSource(sourceMachine);
			sourceMachine = machines[i];
		}
	SetSource(sourceMachine);
}
	
//-----------------------------------------------------------------------------
uint32_t MapChannels::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const inChannels = source->Channels() == 2 ? 2 : 1;
	if (inChannels == outChannels)
		return source->Process(buffer, frames);
	if (inChannels == 1 && outChannels == 2)
	{
		uint32_t const readFrames = source->Process(buffer, frames);
		size_t channelBytes = FramesInBytes<float>(readFrames, 1);
		memcpy(buffer + readFrames, buffer, channelBytes); // ahhh the beauty of deinterlaced channels
		return readFrames;
	}
	if (inChannels == 2 && outChannels == 1)
	{
		if (mixBuffer.Size() < frames * 2)
			mixBuffer.Resize(frames * 2);
		uint32_t const readFrames = source->Process(mixBuffer.Get(), frames);
		float const * leftIn = mixBuffer.Get();
		float const * rightIn = mixBuffer.Get() + readFrames;
		float * out = buffer;
		for (uint_fast32_t i = 0; i < readFrames; ++i)
			*out++ = (*leftIn++ + *rightIn++) * .5; 
		return readFrames;
	}
	return source->Process(buffer, frames);
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

uint32_t LinearFade::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get() || source->Channels() == 0)
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	uint32_t const endA = (startFrame < currentFrame) ? 0 :
		unsigned_min<uint32_t>(readFrames, startFrame - currentFrame);
	uint32_t const endB = (endFrame < currentFrame) ? 0 :
		unsigned_min<uint32_t>(readFrames, endFrame - currentFrame);
	currentFrame += readFrames;
	if (amp == 1 && (endA >= readFrames || endB == 0))
		return readFrames; // amp mignt not be exacly on target, so proximity check would be better	
	float * out = buffer;
	for (uint32_t iChan = 0; iChan < source->Channels(); ++iChan)
	{
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
	return readFrames;
}

//-----------------------------------------------------------------------------	
uint32_t Gain::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	uint32_t const channels = source->Channels();
	float * out = buffer;
	for (uint_fast32_t i = 0; i < readFrames * channels; ++i)
		*out++ *= amp;
	return readFrames;
}
	
//-----------------------------------------------------------------------------
void NoiseSource::Set(uint32_t channels, uint64_t duration)
{
	this->channels = channels == 1 ? 1 : 2;
	this->duration = duration;
	currentFrame = 0;
}

uint32_t NoiseSource::Process(float * const buffer, uint32_t const frames)
{ 
	if (currentFrame > duration)
		return 0;
	float const gah = 1.0 / RAND_MAX;
	const uint32_t framesToWrite = unsigned_min<uint32_t>(duration - currentFrame, frames);
	float * out = buffer;
	for (uint_fast32_t i = 0; i < framesToWrite * channels; ++i)
		*out++ = gah * rand();
	currentFrame += framesToWrite;
	return framesToWrite;
}

//-----------------------------------------------------------------------------
void MixChannels::Set(float llAmp, float lrAmp, float rrAmp, float rlAmp)
{
	this->llAmp = llAmp;
	this->lrAmp = lrAmp;
	this->rrAmp = rrAmp;
	this->rlAmp = rlAmp;
}

uint32_t MixChannels::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	if (source->Channels() != 2)
		return readFrames;
	float * left = buffer;
	float * right = buffer + readFrames;
	for (uint_fast32_t i = 0; i < readFrames; ++i)
	{
		float const newLeft = llAmp * *left + lrAmp * *right;
		float const newRight = rrAmp * *right + rlAmp * *left;
		*left++ = newLeft;
		*right++ = newRight;
	}
	return readFrames;
}

//-----------------------------------------------------------------------------
uint32_t Brickwall::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
//	float * out = buffer;
//	for (uint_fast32_t i = 0; i < readFrames; ++i)
	return readFrames;
}

//-----------------------------------------------------------------------------
uint32_t Peaky::Process(float * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	uint_fast32_t const totalFrames = readFrames * source->Channels();
	float const * in = buffer;
	float max = peak;
	for (uint_fast32_t i = 0; i < totalFrames; ++i)
	{
		float const value = fabs(*in++);
		if (value > max)
			max = value;
	}
	peak = max;
	return readFrames;
}
