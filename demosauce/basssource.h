#ifndef _H_BASSSOURCE_
#define _H_BASSSOURCE_

#include <string>

#include <boost/cstdint.hpp>

#include "bass/bass.h"

void BassSourceInit();

bool BassSourceLoadStream(std::string fileName);
void BassSourceFreeStream();
/**	
*	@return the number of  byts written to buffer.
*/
uint32_t BassSourceFillBufferStream(void * buffer, uint32_t length);

bool BassSourceLoadMusic(std::string fileName);
void BassSourceFreeMusic();
/**	
*	@return the number of  byts written to buffer.
*/
uint32_t BassSourceFillBufferMusic(void * buffer, uint32_t length);

void LogBassError();

#endif
