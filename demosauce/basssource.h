#ifndef _H_BASSSOURCE_
#define _H_BASSSOURCE_

#include <string>

#include <boost/cstdint.hpp>
#include <boost/scoped_ptr.hpp>

#include "dsp.h"

class BassSource : public Machine
{
public:
	BassSource();
	virtual ~BassSource();
	bool Load(std::string fileName, bool prescan = false);
	static bool CheckExtension(std::string fileName);
	
	//overwrite
	uint32_t Process(float * const buffer, uint32_t const frames);
	std::string Name() const {return "Bass Source"; }
	uint32_t Channels() const;
	
	void SetSamplerate(uint32_t moduleSamplerate); // only applies to modules
	void SetLoopDuration(float duration); // only applies to modules
	bool IsModule() const;
	bool IsAmigaModule() const;
	float Loopiness() const;
	
	uint32_t BassChannelType() const;
	uint32_t Samplerate() const;
	uint32_t Bitrate() const;
	double Duration() const;
	
private: 
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

#endif
