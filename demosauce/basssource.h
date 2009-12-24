#ifndef _H_BASSSOURCE_
#define _H_BASSSOURCE_

#include <string>

#include "bass/bass.h"

void BassSourceInit();

bool BassSourceLoadStream(std::string fileName);
void BassSourceFreeStream();
/**	
*	@return the number of  byts written to buffer.
*/
DWORD BassSourceFillBufferStream(void * buffer, DWORD length);

bool BassSourceLoadMusic(std::string fileName);
void BassSourceFreeMusic();
/**	
*	@return the number of  byts written to buffer.
*/
DWORD BassSourceFillBufferMusic(void * buffer, DWORD length);

void LogBassError();

#endif
