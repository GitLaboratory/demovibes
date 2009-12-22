#ifndef _H_SOCKETS_
#define _H_SOCKETS_

#include <boost/asio.hpp>

#include "globals.h"

class Sockets
{
	public:
		Sockets(std::string const & host, std::string const & port);	

		/**  	Tries to obtain the information from the designated host.
		*	Guarantees to return or fail with style (throw some exception)
		*	@param info any content may be overwritten.
		*/
		void GetSong(SongInfo & info);
		
	private:
		/**	Sends a command to designated endpoint.
		*	@param command  command to be sent
		*	@param result 
		*		string is cleared and filled with result of command
		*		content undefined if method returns false
		*	@return true if successfull, false on any error.
		*/
		bool SendCommand(std::string const & command, std::string &  result);
		std::string const host;
		std::string const port;
		boost::asio::io_service io;  
};

#endif
