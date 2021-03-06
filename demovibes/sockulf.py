import socket
import logging
import queuefetcher

Log = logging.getLogger("Sockulf")

class pyWhisperer:
    def __init__(self, host, port, timeout):
        Log.debug("Initiating listener with values HOST='%s', PORT='%s'." % (host, port))
        self.COMMANDS = {
            'GETSONG': self.command_getsong,
            'GETMETA': self.command_getmeta,
            'DIE': self.command_die,
            'GETTITLE': self.command_title,
            'GETARTIST': self.command_artist,
            'MEMDUMP': self.command_memdump,
            'GETGAIN': self.command_getgain,
            'GETLOOP': self.command_getloop,
        }
        self.host = host
        self.port = port
        self.player = queuefetcher.song_finder()
        self.running = True
        self.timeout = timeout
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.bind((self.host, self.port))
        self.listener.settimeout(timeout)

    def listen(self):
        Log.debug("Starting loop")
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

    def command_artist(self):
        return self.player.song.artist().encode("utf-8")

    def command_title(self):
        return self.player.song.title.encode("utf-8")

    def command_getsong(self):
        return self.player.get_next_song()

    def command_getloop(self):
        return str(self.player.song.loopfade_time)

    def command_getgain(self):
        return str(self.player.song.replay_gain)

    def command_getmeta(self):
        return self.player.get_metadata()

    def command_memdump(self):
        try:
            from meliae import scanner
            scanner.dump_all_objects("sockulf.dump")
            return "Memory dumped!"
        except:
            return "Could not dump memory! (meliae not installed?)"

    def command_die(self):
        self.running = False
        return "Goodbye cruel world!"

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-p", "--port", dest="port", default="32167", help = "Which port to listen to")
    parser.add_option("-i", "--ip", dest="ip", default="127.0.0.1", help="What IP address to bind to")
    (options, args) = parser.parse_args()

    HOST = options.ip
    PORT = int(options.port)
    TIMEOUT = None

    logging.basicConfig(level=logging.WARNING)
    Log.setLevel(logging.DEBUG)
    server = pyWhisperer(HOST, PORT, TIMEOUT)
    server.listen()

