#ifndef _H_BASSSOURCE_
#define _H_BASSSOURCE_

#include <string>

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

class BassSource : boost::noncopyable
{
public:
	BassSource();
	virtual ~BassSource();
	bool Load(std::string fileName);
	uint32_t FillBuffer(void * buffer, uint32_t length);
private:
	struct Pimpl;
	boost::scoped_ptr<Pimpl> pimpl;
};

#endif
