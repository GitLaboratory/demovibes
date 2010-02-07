#ifndef _H_AVSOURCE_
#define _H_AVSOURCE_

#include <string>

#include <boost/cstdint.hpp>
#include <boost/scoped_ptr.hpp>

#include "dsp.h"

class AvSource : public Machine
{
public:
	AvSource();
	virtual ~AvSource();
	bool Load(std::string fileName, bool prescan = false);
	static bool CheckExtension(std::string fileName);
	
	//overwrite
	uint32_t Process(float * const buffer, uint32_t const frames);
	std::string Name() const {return "AvCodec Source"; }
	uint32_t Channels() const;
	
	uint32_t AVCodecType() const;
	uint32_t Samplerate() const;
	uint32_t Bitrate() const;
	double Duration() const;

private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

#endif
