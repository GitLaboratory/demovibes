#ifndef _H_DSP_
#define _H_DSP_

#include <cstddef>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/numeric/conversion/cast.hpp>

class Machine
{
public:
	Machine() : bypass(false) {}
	typedef boost::shared_ptr<Machine> MachinePtr;
	
	virtual uint32_t Process(int16_t * const buffer, uint32_t const frames) = 0;
	virtual std::string Name() const = 0;
	virtual uint32_t Channels() const { return source.get() ? source->Channels() : 1; }
	
	void SetSource(MachinePtr & machine) { if (machine.get() != this) source = machine; }
	void SetBypass(bool bypass) { this->bypass = bypass; }
 	bool Bypass() const { return bypass; }
protected:
	MachinePtr source;
	bool bypass;
};

//-----------------------------------------------------------------------------
class MachineStack : public Machine
{
public:
	static size_t const add = static_cast<size_t>(-1);
	MachineStack();
	virtual ~MachineStack();
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Machine Stack"; }
	
	template<typename T> void AddMachine(T & machine, size_t position = add);
	template<typename T> void RemoveMachine(T & machine);
	void UpdateRouting();
private:
	void AddMachine(MachinePtr & machine, size_t position);
	void RemoveMachine(MachinePtr & machine);
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

//-----------------------------------------------------------------------------
class MapChannels : public Machine
{
public:
	MapChannels();
	virtual ~MapChannels();
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Map Channels"; }
	uint32_t Channels() const;
	
	void SetOutChannels(uint32_t outChannels);
private:	
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

//-----------------------------------------------------------------------------
class LinearFade : public Machine
{
public:
	LinearFade();
	virtual ~LinearFade();
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Linear Fade"; }
	
	void Set(uint64_t startFrame, uint64_t endFrame, float beginAmp, float endAmp);
private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

//-----------------------------------------------------------------------------
class Gain : public Machine
{
public:
	Gain() : amp(1) {}
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Gain"; }
	void SetAmp(float amp) { if (amp >= 0) this->amp = amp; }
private:
	float amp;
};

//-----------------------------------------------------------------------------
class NoiseSource : public Machine
{
public:
	NoiseSource() : channels(2), duration(0), currentFrame(0) {}
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Noise"; }
	uint32_t Channels() const { return channels; }
	
	void Set(uint32_t channels, uint64_t duration);
private:
	uint32_t channels;
	uint64_t duration;
	uint64_t currentFrame;
};

//-----------------------------------------------------------------------------
class MixChannels : public Machine
{
public:
	// overwriting
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const { return "Mix Channels"; }
	
	// left = left*llAmp + left*lrAmp; rigt = right*rrAmp + left*rlAmp;
	void Set(float llAmp, float lrAmp, float rrAmp, float rlAmp);
private:
	float llAmp;
	float lrAmp;
	float rrAmp;
	float rlAmp;
};

// class template functions
template<typename T> 
inline void MachineStack::AddMachine (T & machine, size_t position)
{ 
	MachinePtr baseMachine = boost::static_pointer_cast<Machine>(machine);
	AddMachine(baseMachine, position);	
}
template<typename T>
inline void MachineStack::RemoveMachine(T & machine)
{	
	MachinePtr baseMachine = boost::static_pointer_cast<Machine>(machine);
	RemoveMachine(baseMachine);
}

// some helper functions
double DbToAmp(double db);
double AmpToDb(double amp);

template<typename FrameType, typename SampleType, typename ByteType> 
inline FrameType bytes2frames(ByteType const bytes, uint32_t const channels) 
{ return boost::numeric_cast<FrameType>(bytes / sizeof(SampleType) / channels); }

template<typename ByteType, typename SampleType, typename FrameType> 
inline ByteType frames2bytes(FrameType const frames, uint32_t const channels) 
{ return boost::numeric_cast<ByteType>(frames * sizeof(SampleType) * channels); }

template<typename ReturnType> // unsigned min
inline ReturnType unsigned_min(uint64_t const value0, uint64_t const value1)
{ return static_cast<ReturnType>(value0 < value1 ? value0 : value1); }

#endif
