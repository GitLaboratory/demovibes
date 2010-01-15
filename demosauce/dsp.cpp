#include "dsp.h"

#include <cmath>
#include <vector>
#include <algorithm>

#include <boost/numeric/conversion/cast.hpp>

double DbToAmp(double db) { return pow(10, db / 20); }
double AmpToDb(double amp) { return log10(amp) * 20; }
uint64_t SecondsToFrames(double seconds, uint32_t sampleRate) 
	{ return boost::numeric_cast<uint64_t>(seconds * sampleRate); }

//--MachineStack-----------------------------------------

typedef std::vector<Machine::MachinePtr> MachinePtrVector;

struct MachineStack::Pimpl
{
	MachinePtrVector machines;
};

MachineStack::MachineStack() : pimpl(new Pimpl) {}
	
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
	UpdateRouting();
}

void MachineStack::RemoveMachine(MachinePtr machine)
{
	MachinePtrVector & machines = pimpl->machines;
	size_t lastMachine = 0;
	for (size_t i = 0; i < machines.size(); i++)
	{
		if (machines[i] == machine)
			machines[i] = MachinePtr(); // other way to do this?
		if (machines[i])
			lastMachine = i;
	}
	machines.resize(lastMachine + 1);
	UpdateRouting();
}

void MachineStack::UpdateRouting()
{
	MachinePtr lastMachine;
	MachinePtrVector & machines = pimpl->machines;
	for (size_t i = 0; i < machines.size(); i++)
		if (machines[i] && machines[i]->Enabled())
		{
			machines[i]->SetSource(lastMachine);
			lastMachine = machines[i];
		}
	SetSource(lastMachine);
}
	
//--MixChannels-----------------------------------------

struct MixChannels::Pimpl
{
	uint32_t outChannels;
	std::vector<int16_t> mixbuffer;
};

MixChannels::MixChannels() : pimpl(new Pimpl) {}

void MixChannels::ChangeSetting(uint32_t outChannels)
{
	pimpl->outChannels = outChannels == 1 ? 1 : 2;
}

uint32_t MixChannels::Process(int16_t * const buffer, uint32_t const frames)
{
	uint32_t const inChannels = source->Channels() == 1 ? 1 : 2;
	uint32_t const outChannels = pimpl->outChannels;
	if (!source.get() || inChannels == outChannels)
		return frames;
	if (inChannels == 1 && outChannels == 2)
	{
		uint32_t const readFrames = source->Process(buffer, frames / 2);
		int16_t const * in = buffer + readFrames - 1;
		int16_t * outl = buffer + readFrames * 2 - 2; 
		int16_t * outr = buffer + readFrames * 2 - 1;
		for (uint_fast32_t i = 0; i < readFrames; ++i)
		{
			*outl-- = *in;
			*outr-- = *in--;
		}
		return readFrames * 2;
	}
	if (inChannels == 2 && outChannels == 1)
	{
		if (pimpl->mixbuffer.size() < frames * 2)
			pimpl->mixbuffer.resize(frames * 2);
		int16_t * in = & pimpl->mixbuffer[0];
		uint32_t const readFrames = source->Process(in, frames * 2);
		int16_t * out = buffer;
		for (uint_fast32_t i = 0; i < readFrames / 2; ++i)
		{
			int_fast32_t const value = in[0] + in[1]; // need 32 or we get clipping violation
			*out++ = static_cast<int16_t>(value / 2); // any good compiler will potimize this
			in +=2;
		}
		return readFrames / 2;
	}
	return frames;
}

uint32_t MixChannels::Channels() const
{
	return pimpl->outChannels;
}

//--LinearFade------------------------------------------

struct LinearFade::Pimpl
{
	uint64_t startFrame;
	uint64_t endFrame;
	uint64_t currentFrame;
	double amp;
	double ampInc;
};

LinearFade::LinearFade() : pimpl(new Pimpl) {}
	
void LinearFade::ChangeSetting(uint64_t startFrame, uint64_t endFrame, float beginAmp, float endAmp)
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
		std::min(pimpl->startFrame - pimpl->currentFrame, static_cast<uint64_t>(frames));
	uint32_t const endB = (pimpl->endFrame < pimpl->currentFrame) ? 0 :
		std::min(pimpl->endFrame - pimpl->currentFrame, static_cast<uint64_t>(frames));
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

//--Gain------------------------------------------------
	
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
	
//--NoiseGenerator--------------------------------------

void NoiseGenerator::ChangeSetting(uint32_t channels, uint64_t duration)
{
	this->channels = channels == 1 ? 1 : 2;
	this->duration = duration;
	currentFrame = 0;
}

uint32_t NoiseGenerator::Process(int16_t * const buffer, uint32_t const frames)
{ 
	if (currentFrame > duration)
		return frames;
	const uint32_t framesToWrite = std::min(duration - currentFrame, static_cast<uint64_t>(frames));
	int16_t * out = buffer;
	for (size_t i = 0; i < framesToWrite * channels; ++i)
		*out++ = static_cast<int16_t>(rand() & 0x00ff);
	currentFrame += framesToWrite;
	return framesToWrite;
}
