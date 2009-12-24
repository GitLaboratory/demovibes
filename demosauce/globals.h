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
	extern std::string	demovibes_port;
	extern uint32_t		demovibes_port_int;
	extern std::string	encoder_samplerate;
	extern uint32_t		encoder_samplerate_int;
	extern std::string	encoder_bitrate;
	extern uint32_t		encoder_bitrate_int;
	extern std::string	cast_host;
	extern std::string	cast_port;
	extern uint32_t		cast_port_int;
	extern std::string	cast_mount;
	extern std::string	cast_password;
	extern std::string	cast_name;
	extern std::string	cast_url;
	extern std::string	cast_genre;
	extern std::string	cast_description;
}

// definitons

struct SongInfo
{
	std::string fileName;
	std::string title;
};

// forward declaration for obtaining playing song info
SongInfo GetNextSong();

#endif
