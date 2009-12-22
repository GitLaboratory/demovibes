#include <boost/thread/thread.hpp>
#include <boost/format.hpp>

#include "globals.h"
#include "sockets.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using boost::asio::ip::tcp;

Sockets::Sockets(string const & host, string const & port):
	host(host),
	port(port)
{
}

bool 
Sockets::SendCommand(string const & command, string &  result)
{
	result.clear(); // result string might conatin shit from lasst call or something
	try
	{
		// create endpoint for address + port
		tcp::resolver resolver(io);
		tcp::resolver::query query(host, port);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		tcp::socket socket(io);
		// find fire suitable endpoint
		system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end)
		{
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}
		if (error)
			throw boost::system::system_error(error);
		// write command to socket
		write(socket,  buffer(command), transfer_all(), error);
		if (error)
			throw boost::system::system_error(error);
		for (;;)
		{
			boost::array<char, 256> buf;
			boost::system::error_code error;
			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			result.append(buf.data(), len);
		}
		logg << format("INFO: socket command=%1% result=%2%\n")  % command % result;
	}
	catch (std::exception & e)
	{
		logg  << "ERROR: " << e.what() << endl;
		return false;
	}
	return true;
}

void 
Sockets::GetSong(SongInfo & info)
{
	size_t attempts = 3;
	// since this is critical we try multiple times
	while (attempts-- != 0 && !SendCommand("GETSONG", info.fileName))
	{
		logg << "INFO: sleeping a second\n";
		this_thread::sleep(posix_time::seconds(1));
	}
	if (attempts == 0)
	{
		logg << "ERROR: too many attempts\n";
		exit(666);
	}
	if (!SendCommand("GETMETA", info.title))
		logg << "WARNING: failed to get title for " << info.fileName << endl;
	
}
