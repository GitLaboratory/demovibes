#include <cmath>
#include <vector>
#include <algorithm>

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
	
uint32_t MachineStack::Process(int16_t * const buffer, uint32_t const frames)
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
struct MapChannels::Pimpl
{
	uint32_t outChannels;
	std::vector<int16_t> mixbuffer;
};

MapChannels::MapChannels() : pimpl(new Pimpl) { pimpl->outChannels = 2; }
MapChannels::~MapChannels() {}

void MapChannels::SetOutChannels(uint32_t outChannels)
{
	pimpl->outChannels = outChannels == 1 ? 1 : 2;
}

uint32_t MapChannels::Process(int16_t * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const inChannels = source->Channels() == 1 ? 1 : 2;
	uint32_t const outChannels = pimpl->outChannels;
	if (inChannels == outChannels)
		return source->Process(buffer, frames);
	if (inChannels == 1 && outChannels == 2)
	{
		uint32_t const readFrames = source->Process(buffer, frames);
		int16_t const * in = buffer + readFrames - 1;
		int16_t * outl = buffer + readFrames * 2 - 2; 
		int16_t * outr = buffer + readFrames * 2 - 1;
		for (uint_fast32_t i = 0; i < readFrames; ++i)
		{
			*outl = *in; *outr = *in--; 
			outl -= 2; outr -= 2;
		}
		return readFrames;
	}
	if (inChannels == 2 && outChannels == 1)
	{
		if (pimpl->mixbuffer.size() < frames * 2)
			pimpl->mixbuffer.resize(frames * 2);
		int16_t * in = & pimpl->mixbuffer[0];
		uint32_t const readFrames = source->Process(in, frames);
		int16_t * out = buffer;
		for (uint_fast32_t i = 0; i < readFrames / 2; ++i)
		{
			int_fast32_t const value = in[0] + in[1]; // need 32 or we get clipping violation
			*out++ = static_cast<int16_t>(value / 2); // any good compiler will potimize this
			in += 2;
		}
		return readFrames;
	}
	return source->Process(buffer, frames);
}

uint32_t MapChannels::Channels() const
{
	return source.get() ? pimpl->outChannels : 1;
}

//-----------------------------------------------------------------------------
struct LinearFade::Pimpl
{
	uint64_t startFrame;
	uint64_t endFrame;
	uint64_t currentFrame;
	double amp;
	double ampInc;
};

LinearFade::LinearFade() : pimpl(new Pimpl) {}
LinearFade::~LinearFade() {}
	
void LinearFade::Set(uint64_t startFrame, uint64_t endFrame, float beginAmp, float endAmp)
{
	if (startFrame >= endFrame || beginAmp < 0 || endAmp < 0) 
		return;	
	pimpl->startFrame = startFrame;
	pimpl->endFrame = endFrame;
	pimpl->currentFrame = 0;
	pimpl->amp = beginAmp;
	pimpl->ampInc = (endAmp - beginAmp) / (endFrame - startFrame);
}

uint32_t LinearFade::Process(int16_t * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	uint32_t const channels = source->Channels();
	uint32_t const endA = (pimpl->startFrame < pimpl->currentFrame) ? 0 :
		unsigned_min<uint32_t>(frames, pimpl->startFrame - pimpl->currentFrame);
	uint32_t const endB = (pimpl->endFrame < pimpl->currentFrame) ? 0 :
		unsigned_min<uint32_t>(frames, pimpl->endFrame - pimpl->currentFrame);
	float amp = pimpl->amp;
	float const ampInc = pimpl->ampInc / channels;
	int16_t * out = buffer;
	uint_fast32_t i = 0;	
	for (; i < endA * channels; ++i)
		*out++ *= amp;
	for (; i < endB * channels; ++i)
	{
		*out++ *= amp;
		amp += ampInc;
	}
	for (; i < readFrames * channels; ++i)
		*out++ *= amp;
	pimpl->amp += pimpl->ampInc * (endB - endA);
	return readFrames;
}

//-----------------------------------------------------------------------------	
uint32_t Gain::Process(int16_t * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	uint32_t const channels = source->Channels();
	int16_t * out = buffer;
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

uint32_t NoiseSource::Process(int16_t * const buffer, uint32_t const frames)
{ 
	if (currentFrame > duration)
		return 0;
	const uint32_t framesToWrite = std::min(duration - currentFrame, static_cast<uint64_t>(frames));
	int16_t * out = buffer;
	for (uint_fast32_t i = 0; i < framesToWrite * channels; ++i)
		*out++ = static_cast<int16_t>(rand() & 0x00ff);
	currentFrame += framesToWrite;
	return framesToWrite;
}

//-----------------------------------------------------------------------------

uint32_t MixChannels::Process(int16_t * const buffer, uint32_t const frames)
{
	if (!source.get())
		return frames;
	uint32_t const readFrames = source->Process(buffer, frames);
	if (source->Channels() != 2)
		return readFrames;
	int16_t const * in = buffer;
	int16_t * out = buffer;
	for (uint_fast32_t i = 0; i < readFrames; ++i)
	{
		int16_t const left = *in++;
		int16_t const right = *in++;
		*out++ = static_cast<int16_t>(llAmp * left + lrAmp * right);
		*out++ = static_cast<int16_t>(rrAmp * right + rlAmp * left);
	}
	return readFrames;
}	

float llAmp;
float lrAmp;
float rrAmp;
float rlAmp;
