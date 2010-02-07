#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#include "globals.h"
#include "logror.h"
#include "sockets.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace logror;
using boost::asio::ip::tcp;

struct Sockets::Pimpl
{
	bool SendCommand(string const & command, string &  result);
	string host;
	uint32_t port;
	io_service io;
};

Sockets::Sockets(string const & host, uint32_t const port):
	pimpl(new Pimpl)
{
	pimpl->host = host;
	pimpl->port = port;
}

bool 
Sockets::Pimpl::SendCommand(string const & command, string & result)
{
	result.clear(); // result string might conatin shit from lasst call or something
	try
	{
		// create endpoint for address + port
		tcp::resolver resolver(io);
		tcp::resolver::query query(host, lexical_cast<string>(port));
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
		LogDebug("socket command=%1% result=%2%"), command, result;
	}
	catch (std::exception & e)
	{
		Log(warning ,"%1%"), e.what();
		return false;
	}
	return true;
}

void 
Sockets::GetSong(SongInfo & info)
{
	if (!pimpl->SendCommand("GETSONG", info.fileName))
	{
		Error("socket command GETSONG failed");
		info.fileName = setting::error_tune;
	}
	
	if (!pimpl->SendCommand("GETMETA", info.title))
		info.title = setting::error_title;
	
	string gain;
	if (!pimpl->SendCommand("GETGAIN", gain))
		gain = "0"; // right, gain in db
	try { info.gain = lexical_cast<float>(gain); }
	catch (bad_lexical_cast &) { info.gain = 0; }
	
	string loopDuration;
	if (!pimpl->SendCommand("GETLOOP", loopDuration))
		loopDuration = "0";
	try { info.loopDuration = lexical_cast<float>(gain); }
	catch (bad_lexical_cast &) { info.loopDuration = 0; }
}

bool ResolveIp(string host, std::string &ipAddress)
{
	try
	{
		io_service io;
		tcp::resolver resolver(io);
		tcp::resolver::query query(tcp::v4(), host, "0");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		if (endpoint_iterator == end)
			return false;
		tcp::endpoint ep = *endpoint_iterator;
		ipAddress = ep.address().to_string();
	}
	catch (std::exception & e)
	{
		Error("%1%"), e.what();
		return false;
	}
	return true;
}

Sockets::~Sockets() {} // empty, to make scoped_ptr happy
