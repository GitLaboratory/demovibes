#ifndef _H_DSP_
#define _H_DSP_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

double DbToAmp(double db);
double AmpToDb(double amp);
uint64_t SecondsToFrames(double seconds, uint32_t sampleRate);

class Machine
{
public:
	typedef boost::shared_ptr<Machine> MachinePtr;
	virtual uint32_t Process(int16_t * const buffer, uint32_t const frames) = 0;
	void SetSource(MachinePtr & machine) { if (machine.get() != this) source = machine; }
	void Enabled(bool enabled) { this->enabled = enabled; }
 	bool Enabled() const { return enabled; }
 	virtual uint32_t Channels() const { return source.get() ? source->Channels() : 0; }
protected:
	MachinePtr source;
	bool enabled;
};

class MachineStack : Machine
{
public:
	static size_t const add = static_cast<size_t>(-1);
	MachineStack();
	virtual ~MachineStack() {} // needed or scoped_ptr may start whining
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	void AddMachine(MachinePtr & machine, size_t position = add);
	void RemoveMachine(MachinePtr machine);
	void UpdateRouting();
private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};
 
class MixChannels : Machine
{
public:
	MixChannels();
	virtual ~MixChannels();
	void ChangeSetting(uint32_t outChannels);
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	uint32_t Channels() const;
private:	
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

class LinearFade : Machine
{
public:
	LinearFade();
	virtual ~LinearFade() {}
	void ChangeSetting(uint64_t startFrame, uint64_t endFrame, float beginAmp, float endAmp);
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

class Gain : Machine
{
public:
	void ChangeSetting(float amp) { if (amp >= 0) this->amp = amp; }
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
private:
	float amp;
};

class NoiseGenerator : Machine
{
public:
	void ChangeSetting(uint32_t channels, uint64_t duration);
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	uint32_t Channels() const { return channels; }
private:
	uint32_t channels;
	uint64_t duration;
	uint64_t currentFrame;
};

#endif
