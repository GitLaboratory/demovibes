#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "globals.h"

using namespace std;
using namespace boost::program_options;

namespace setting
{
	string demovibes_host		= "localhost";
	uint32_t demovibes_port		= 32167;
	uint32_t encoder_samplerate	= 44100;
	uint32_t encoder_bitrate	= 192;
	string cast_host			= "localhost";
	uint32_t cast_port			= 6010;
	string cast_mount			= "ices";
	string cast_password;
	string cast_name;
	string cast_url;
	string cast_genre;
	string cast_description;
	string error_tune;
	string error_title			= "sorry, we're having some trouble";
}

using namespace setting;

string configFileName = "demosauce.conf";
string castForcePassword;

void BuildDescriptions
(
	options_description & settingsDesc,
	options_description & optionsDesc
)
{
	settingsDesc.add_options()
	("demovibes_host", value<string>(&demovibes_host))
    ("demovibes_port", value<uint32_t>(&demovibes_port))
	("encoder_samplerate", value<uint32_t>(&encoder_samplerate))
	("encoder_bitrate", value<uint32_t>(&encoder_bitrate))
	("cast_host", value<string>(&cast_host))
	("cast_port", value<uint32_t>(&cast_port))
	("cast_mount", value<string>(&cast_mount))
	("cast_password", value<string>(&cast_password))
	("cast_name", value<string>(&cast_name))
	("cast_url", value<string>(&cast_url))
	("cast_genre", value<string>(&cast_genre))
	("cast_description", value<string>(&cast_description))
	("error_tune", value<string>(&error_tune))
	("error_title", value<string>(&error_title))
	;

	optionsDesc.add_options()
	("help", "what do you think? make a pizza maybe?")
	("config_file,c", value<string>(&configFileName), "use config file, default: demosauce.conf")
	("cast_password,p", value<string>(&castForcePassword), "password for cast server, overwrites setting from config file")
	;
}

void InitSettings(int argc, char* argv[])
{
	options_description settingsDesc("Settings");
	options_description optionsDesc("Allowed options");
	BuildDescriptions(settingsDesc, optionsDesc);
	
	variables_map optionsMap;
	store(parse_command_line(argc, argv, optionsDesc), optionsMap);
	notify(optionsMap);    

	if (optionsMap.count("help")) 
	{
		cout << optionsDesc << "\n";
		exit(EXIT_SUCCESS);
	}
	
	if (!boost::filesystem::exists(configFileName))
	{
		std::cout << "cannot find config file: " << configFileName << endl;
		exit(EXIT_FAILURE);
	}

	ifstream configFile(configFileName.c_str(), ifstream::in);
	if (configFile.fail())
	{
		std::cout << "failed to open config file: " << configFileName << endl;
		exit(EXIT_FAILURE);
	}
	cout << "reading config file\n";
	variables_map settingsMap;
	store(parse_config_file(configFile, settingsDesc), settingsMap);
	notify(settingsMap);
	
	if (optionsMap.count("cast_password")) 
	{
		cast_password = castForcePassword;
	}
}