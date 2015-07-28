# Introduction #

A short, basic example on how one can override existing functions, and add eventual new functions.

This subclasses and changes the logic in queuefetcher.py and sockulf.py

# Details #

```
import sockulf

class MyFetcher(sockulf.queuefetcher.song_finder):

    def isGoodSong(self, song):
        print "It is TRUE! GOOD SONG"
        return True # All songs are good songs!

class mySock(sockulf.pyWhisperer):
    def command_mycustomcommand(self): #We need a new, custom command
        return "Hahey, my custom command! WOO!"

    def __init__(self, *args, **kwargs):
        super(mySock, self).__init__(*args, **kwargs)
        self.player = MyFetcher() #override player with my own after running the original init
        self.COMMANDS['MYCUSTOM'] = self.command_mycustomcommand # Add custom command to dict of command -> function mapping

#Lets start the damn thing
#Just as in sockulf - but without the cute option parser part
server = mySock("127.0.0.1", 32167, None)
server.listen()
```