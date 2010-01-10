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

#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "globals.h"
#include "sockets.h"
#include "basscast.h"
#include "settings.h"
#include "basssource.h"

using namespace std;
using namespace boost;
using namespace logror;

Sockets * demovibes = NULL;

SongInfo GetNextSong()
{
	if (demovibes == NULL)
		Fatal("kill your coder"); // should never happen
	SongInfo info;
	demovibes->GetSong(info);
	return info;
}

int main(int argc, char* argv[])
{
	cout << "demosauce 0.13\n";
	try
	{
		InitSettings(argc, argv);
		LogSetConsoleLevel(setting::log_console_level);
		LogSetFile(setting::log_file, setting::log_file_level);
		Sockets socktes(setting::demovibes_host, setting::demovibes_port);
		demovibes = &socktes;
		BassCastInit();
		cout << "runining...\n";
		BassCastRun();
	} 
	catch (std::exception & e)
	{
		Fatal("%1%"), e.what();
	}
	return EXIT_SUCCESS;
}
