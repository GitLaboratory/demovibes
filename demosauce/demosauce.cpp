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
#include <iostream>

#include "globals.h"
#include "settings.h"
#include "basscast.h"

using namespace logror;

int main(int argc, char* argv[])
{
	std::cout << "demosauce 0.14 - Now with MORE flavor!\n";
	try
	{
		InitSettings(argc, argv);
		LogSetConsoleLevel(setting::log_console_level);
		LogSetFile(setting::log_file, setting::log_file_level);
		BassCast cast;
		std::cout << "runniALL GLORY TO THE HYPNOTOAD!\n";
		cast.Run();
	} 
	catch (std::exception & e)
	{
		Fatal("%1%"), e.what();
	}
	return EXIT_SUCCESS;
}
