#ifndef _H_GLOBALS_
#define _H_GLOBALS_

#include <string>
#include <fstream>
#include <iostream>

#include <boost/cstdint.hpp>

// variables
extern std::ofstream logg; // log instanciated in demosauce.cpp

namespace setting 
{
	extern std::string	demovibes_host;
	extern uint32_t		demovibes_port;
	extern uint32_t		encoder_samplerate;
	extern uint32_t		encoder_bitrate;
	extern std::string	cast_host;
	extern uint32_t		cast_port;
	extern std::string	cast_mount;
	extern std::string	cast_password;
	extern std::string	cast_name;
	extern std::string	cast_url;
	extern std::string	cast_genre;
	extern std::string	cast_description;
	extern std::string  error_tune;
	extern std::string	error_title;
}

// definitons
#define SAMPLE_SIZE 2
struct SongInfo
{
	std::string fileName;
	std::string title;
};

// function declarations
SongInfo GetNextSong();

#endif
