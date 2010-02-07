
// will be reenabled once ffmpeg is supported fully 
//extern "C" {
//#include <libavutil/common.h>
//}

#include <cstdlib>

#include "misc.h"

void* _Realloc(void* buffer, size_t size)
{
	//return av_realloc(buffer, size);
	return realloc(buffer, size);
}

void _Free(void* buffer)
{
	//av_free(buffer);
	free(buffer);
}
