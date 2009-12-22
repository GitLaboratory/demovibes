#ifndef _H_GLOBALS_
#define _H_GLOBALS_

#include <string>
#include <fstream>
#include <iostream>

// variables
extern std::ofstream logg; // log instanciated in demosauce.cpp

namespace setting 
{
	extern std::string demovibes_host;
	extern std::string demovibes_port;
	extern std::string encoder_samplerate;
	extern std::string encoder_bitrate;
	extern std::string cast_host;
	extern std::string cast_password;
	extern std::string cast_name;
	extern std::string cast_url;
	extern std::string cast_genre;
	extern std::string cast_description;
}

// definitons

struct SongInfo
{
	std::string fileName;
	std::string title;
};

enum DecoderType
{
	nada,
	codec_generic,
	module_generic,
	module_amiga
};

// some usefull functions
DecoderType DecideDecoderType(std::string const & fileName);
SongInfo GetSong();

#endif
