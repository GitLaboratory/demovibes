#ifndef _H_DECODER_COMMON_
#define _H_DECODER_COMMON_

#include <string>
#include <boost/algorithm/string.hpp>

#include "logror.h"

enum DecoderType
{
	decoder_nada,
	decoder_noise,
	decoder_codec_generic,
	decoder_module_generic,
	decoder_module_amiga
};

inline DecoderType 
DecideDecoderType(const std::string & fileName)
{
	DecoderType type = decoder_nada;
	if (boost::iends_with(fileName, ".mp3") ||
		boost::iends_with(fileName, ".mp2") ||
		boost::iends_with(fileName, ".mp1") ||
		boost::iends_with(fileName, ".ogg") ||
		boost::iends_with(fileName, ".aac") ||
		boost::iends_with(fileName, ".m4a"))
		type = decoder_codec_generic;
	else if (boost::iends_with(fileName, ".xm") ||
		boost::iends_with(fileName, ".s3m") ||
		boost::iends_with(fileName, ".it"))
		type = decoder_module_generic;
	else if (boost::iends_with(fileName, ".mod"))
		type = decoder_module_amiga;
	else
		logror::Error("no decoder for %1%"), fileName;
	return type;
}

#endif