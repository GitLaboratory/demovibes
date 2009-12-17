import socket
import logging
import pyAdder


Log = logging.getLogger("Sockulf")

HOST = "127.0.0.1"
PORT = 32167
TIMEOUT = None #Seconds, floating point

class pyWhisperer:
	def __init__(self, host, port, timeout):
		Log.debug("Initiating listener with values HOST='%s', PORT='%s', TIMEOUT='%s'." % (host, port, timeout))
		self.COMMANDS = {
			'GETSONG': self.command_getsong,
			'GETMETA': self.command_getmeta,
			'DIE': self.command_die,
		}
		self.host = host
		self.port = port
		self.running = True
		self.timeout = timeout
		self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.listener.bind((self.host, self.port))
		self.listener.settimeout(timeout)

	def listen(self):
		Log.debug("Starting loop")
		pyAdder.ices_init()
		while self.running:
			self.listener.listen(1)
			Log.debug("Listening for connections..")
			self.conn, self.addr = self.listener.accept()
			Log.debug("Accepted connection from %s" % self.addr[0])
			data = self.conn.recv(1024)
			i = 0
			if data:
				data = data.strip()
				Log.debug("Got message : %s" % data)
				if data in self.COMMANDS.keys():
					result = self.COMMANDS[data]()
					Log.debug("Returning data : %s" % result)
					while i < len(result):
						i = i + self.conn.send(result)
				else:
					Log.debug("Unknown command!")
			else:
				Log.debug("No data received")
			Log.debug("Closing connection")
			self.conn.close()
		Log.debug("Closing down listener")
		self.listener.close()
		pyAdder.ices_shutdown()

	def command_getsong(self):
		return pyAdder.ices_get_next()

	def command_getmeta(self):
		return pyAdder.ices_get_metadata()

	def command_die(self):
		self.running = False
		return "Goodbye cruel world!"

if __name__ == '__main__':
	import sys
	
	from optparse import OptionParser
	
	logging.basicConfig(level=logging.WARNING)
	Log.setLevel(logging.DEBUG)
	server = pyWhisperer(HOST, PORT, TIMEOUT)
	server.listen()
