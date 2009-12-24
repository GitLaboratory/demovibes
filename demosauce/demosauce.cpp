/*
	fancy streaming engine for scenemusic 
	slapped together by maep, [put your names here]  2009
	
	HERE BE PONIES!
	
*/

#include <string>
#include <fstream>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#include "globals.h"
#include "sockets.h"
#include "basscast.h"
#include "basssource.h"

using namespace std;
using namespace boost;

// global stuff
ofstream logg("demosauce.log");

// some settings hardcoded for now, will be read from file later
string setting::demovibes_host				= "localhost";
string setting::demovibes_port				= "32167";
uint32_t setting::demovibes_port_int		= 32167;
string setting::encoder_samplerate			= "44100";
uint32_t setting::encoder_samplerate_int	= 44100;
string setting::encoder_bitrate				= "192";
uint32_t setting::encoder_bitrate_int		= 192;
string setting::cast_host					= "localhost";
string setting::cast_port					= "6010";
uint32_t setting::cast_port_int				= 6010;
string setting::cast_mount					= "ices";
string setting::cast_password				= "iUqAv11CHAY";
string setting::cast_name					= "Nectarine";
string setting::cast_url					= "http://scenemusic.eu/";
string setting::cast_genre					= "scenemusic";
string setting::cast_description 			= "Demoscene Radio";

Sockets * demovibes = NULL;

SongInfo GetNextSong()
{
	if (demovibes == NULL)
	{
		logg << "ADVICE: kill your coder\n"; // should never happen
		exit(666);
	}
	SongInfo info;
	demovibes->GetSong(info);
	return info;
}

void InitSettings()
{
	// TODO: read config file
	try
	{
		setting::demovibes_port_int = lexical_cast<uint32_t>(setting::demovibes_port);
		setting::encoder_samplerate_int = lexical_cast<uint32_t>(setting::encoder_samplerate);
		setting::encoder_bitrate_int = lexical_cast<uint32_t>(setting::encoder_bitrate);
		setting::cast_port_int	= lexical_cast<uint32_t>(setting::cast_port);
	}
	catch (std::exception & e)
	{
		cout << "something is wrong with your settings, probably a letter in a number\n";
		exit(666);
	}
}

const char * busy = "demoosauce(.Y.)";

int main(int argc, char* argv[])
{
	cout << "demosauce 0.11\ncheck demosauce.log for info/errors\n";
	try
	{
		InitSettings();
		Sockets socktes(setting::demovibes_host, setting::demovibes_port);
		demovibes = &socktes;
		BassSourceInit();
		BassCastInit();
		cout << "runining...\n";
		size_t aniIndex = 0;
		while (true)
		{
			Test();
			if (aniIndex % 5 == 0)
				cout << "\b\b\b\b\b";
			cout << busy[aniIndex] << flush;
			aniIndex = (aniIndex + 1) % 15;
			this_thread::sleep(posix_time::seconds(1));
		}
	} 
	catch (std::exception & e)
	{
		logg << "ERROR: " << e.what() << endl;
		exit(666);
	}
	return 0;
}
