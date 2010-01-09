import socket
import logging

logging.basicConfig(level=logging.DEBUG)
Log = logging

class locoland:
    def __init__(self, host = "127.0.0.1", port = 9911):
        self.events = []
        self.eventid = 0
        self.waiters = []
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((host, port))
        Log.debug("Set up for host %s and port %s" % (host, port))
        self.max_events = 20
        self.on = True

    def add_event(self, event):
        Log.debug("add_event : %s" % event)
        self.eventid += 1
        if not event in self.events:
            self.events.insert(0, event)
            self.events = self.events[0:self.max_events]
            self.send_events()

    def send_events(self):
	Log.debug("Sending events to %s clients" % len(self.waiters))
        message = self.mk_message()
        while self.waiters:
            sock = self.waiters.pop()
            self.send_message(sock, message)
            del sock

    def mk_message(self):
        message = "\n".join(self.events) + "\n!%s" % self.eventid
        Log.debug("Make message: %s" % message)
        return message

    def send_message(self, sock, message = False):
        Log.debug("Sending message to socket")
        if not message:
            message = self.mk_message()
        ml = len(message)
        sent = 0
        while ml > sent:
            sent += sock.send(message[sent:ml])
        sock.close()


    def listen(self):
        while self.on:
            Log.debug("Listening for new connection")
            self.sock.listen(1)
            conn, addr = self.sock.accept()
            key, data = conn.recv(1024).strip().split(":", 1)
            Log.debug("Got a new connection with key '%s' and data '%s'"% (key, data))
            if key == "get":
                if int(data) < self.eventid:
                    self.send_message(conn)
                else:
                    Log.debug("Adding connecton to waiter list")
                    self.waiters.append(conn)
            if key == "set":
                self.add_event(data)
		conn.close()
            if key == "die":
                Log.debug("Dying")
                self.on = False

if __name__ == '__main__':
	moo = locoland()
	moo.listen()
