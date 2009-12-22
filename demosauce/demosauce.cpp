/*
	fancy streaming engine for scenemusic 
	slapped together by maep, [put your names here]  2009
	
	HERE BE PONIES!
	
*/

#include <string>
#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include "globals.h"
#include "sockets.h"
#include "bassstuff.h"

using namespace std;
using namespace boost;

// global stuff
ofstream logg("demosauce.log");

// some settings hardcoded for now, will be read from file later
string setting::demovibes_host 		= "localhost:8000";
string setting::demovibes_port 		= "32167";
string setting::encoder_samplerate 	= "44100";
string setting::encoder_bitrate 	= "192";
string setting::cast_host 		= "localhost";
string setting::cast_password 		= "iUqAv11CHAY";
string setting::cast_name 		= "Nectarine";
string setting::cast_url 		= "http://scenemusic.eu/";
string setting::cast_genre 		= "scenemusic";
string setting::cast_description 	= "Demoscene Radio";

DecoderType DecideDecoderType(const string & fileName)
{
	DecoderType type = nada;
	const string & name = fileName;
	if (iends_with(name, ".mp3") ||
		iends_with(name, ".mp2") ||
		iends_with(name, ".mp1") ||
		iends_with(name, ".ogg") ||
		iends_with(name, ".aac") ||
		iends_with(name, ".m4a"))
		type = codec_generic;
	else if (iends_with(name, ".xm") ||
		iends_with(name, ".s3m") ||
		iends_with(name, ".it"))
		type = module_generic;
	else if (iends_with(name, ".mod"))
		type = module_amiga;
	return type;
}

Sockets * demovibes = NULL;

SongInfo GetSong()
{
	if (demovibes == NULL)
	{
		logg << "ERROR: kill your coder\n";
		exit(666);
	}
	SongInfo info;
	demovibes->GetSong(info);
	return info;
}

int main(int argc, char* argv[])
{
	try
	{
		// TODO: read config file
		Sockets socktes(setting::demovibes_host, setting::demovibes_port);
		demovibes = &socktes;
		Bass_Init();
		cout << "demosauce 0.1\nPress <enter> to exit. Everything is logged to demosauce.log.\n";
		string dummy;
		cin >> dummy; // wait for return
	} 
	catch (std::exception & e)
	{
		logg << "ERROR: " << e.what() << endl;
		exit(666);
	}
	return 0;
}
