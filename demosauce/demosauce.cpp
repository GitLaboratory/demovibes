/*
	fancy streaming engine for scenemusic 
	slapped together by maep, [put your names here]  2009
	
	BEHOLD, A FUCKING PONY!
	
	           .,,.
         ,;;*;;;;,
        .-'``;-');;.
       /'  .-.  /;;;
     .'    \d    \;;               .;;;,
    / o      `    \;    ,__.     ,;*;;;*;,
    \__, _.__,'   \_.-') __)--.;;;;;*;;;;,
     `""`;;;\       /-')_) __)  `\' ';;;;;;
        ;*;;;        -') `)_)  |\ |  ;;;;*;
        ;;;;|        `---`    O | | ;;*;;;
        *;*;\|                 O  / ;;;;;*
       ;;;;;/|    .-------\      / ;*;;;;;
      ;;;*;/ \    |        '.   (`. ;;;*;;;
      ;;;;;'. ;   |          )   \ | ;;;;;;
      ,;*;;;;\/   |.        /   /` | ';;;*;
       ;;;;;;/    |/       /   /__/   ';;;
       '*jgs/     |       /    |      ;*;
            `""""`        `""""`     ;'
	
	pony source:
	http://www.geocities.com/SoHo/7373/index.htm#home
	(yeah... geocities, haha)
*/

#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>

#include "globals.h"
#include "sockets.h"
#include "basscast.h"
#include "basssource.h"
#include "settings.h"

using namespace std;
using namespace boost;

// global stuff
ofstream logg("demosauce.log");

Sockets * demovibes = NULL;

SongInfo GetNextSong()
{
	if (demovibes == NULL)
	{
		logg << "ADVICE: kill your coder\n"; // should never happen
		exit(EXIT_FAILURE);
	}
	SongInfo info;
	demovibes->GetSong(info);
	return info;
}

int main(int argc, char* argv[])
{
	cout << "demosauce 0.12\ncheck demosauce.log for info/errors, also now we have --help \\o/\n";
	try
	{
		InitSettings(argc, argv);
		Sockets socktes(setting::demovibes_host, setting::demovibes_port);
		demovibes = &socktes;
		BassSourceInit();
		BassCastInit();
		cout << "runining...\n";
		BassCastRun();
	} 
	catch (std::exception & e)
	{
		logg << "ERROR: " << e.what() << endl;
		cout << e.what() << endl;
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
