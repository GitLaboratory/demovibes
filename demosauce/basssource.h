#ifndef _H_BASSSOURCE_
#define _H_BASSSOURCE_

#include <string>

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

#include "dsp.h"

class BassSource : public Machine, boost::noncopyable
{
public:
	BassSource();
	virtual ~BassSource();
	//overwrite
	bool Load(std::string fileName, bool prescan = false);
	uint32_t Process(int16_t * const buffer, uint32_t const frames);
	std::string Name() const {return "Bass Source"; }
	
	void SetSamplerate(uint32_t moduleSamplerate);
	void SetLoopvogel(float duration);
	
	uint32_t BassChannelType() const;
	uint32_t Channels() const;
	uint32_t Samplerate() const;
	uint32_t Bitrate() const;
	float Duration() const;
	
private: 
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

#endif
