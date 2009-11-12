import sys, random, os
from datetime import datetime, timedelta
import time
from os import popen

from django.core.management import setup_environ
import settings
setup_environ(settings)
from webview.models import *
from django.contrib.auth.models import User
from webview import common
from string import *

enc = sys.getdefaultencoding()
fsenc = sys.getfilesystemencoding()

dj_username = "djrandom"
twitter_username = ""
twitter_password = ""

try:
    djUser = User.objects.get(username = dj_username)
except:
    print "ERROR : User '%s' does not exist! Please create that user or change user in pyAdder. Can not start!" % dj_username
    sys.exit(1)

meta = None
timestamp = None

# Jingles variable setup
jt_count = 0
jt_timelast = datetime.datetime.now()
jt_max = timedelta(minutes = 30)
jt_min = timedelta(minutes = 20)

#Good song weighting
# N is "No votes / fewer than 5 votes"
# The "weight" just means that the system will try to
# play X songs until it play one from that weight class again.
songweight = {
        'N' : 1,
        1 : 40,
        2 : 25,
        3 : 10,
        4 : 3,
        5 : 1,
    }

Weight = {
        'N' : 0,
        1 : 0,
        2 : 0,
        3 : 0,
        4 : 0,
        5 : 0
    }


# Function called to initialize your python environment.
# Should return 1 if ok, and 0 if something went wrong.

def ices_init ():
        return 1

# Function called to shutdown your python enviroment.
# Return 1 if ok, 0 if something went wrong.
def ices_shutdown ():
        return 1

# Function called to get the next filename to stream.
# Should return a string.
def ices_get_next ():
    global meta
    global timestamp
    global twitter_username
    global twitter_password

    if timestamp:
        delta = datetime.datetime.now() - timestamp
        if delta < timedelta(seconds=3):
            time.sleep(3)
            print "ERROR : Song '%s' borked for some reason!" % meta
    timestamp = datetime.datetime.now()

    song = findQueued()

    meta = "%s - %s" % (song.artist(), song.title)
    print "Now playing", song.file.path.encode(enc)
    twitter_message = "Now playing: %s - %s" % (song.artist(), song.title)
    if len(twitter_username) > 0:
	tweet(twitter_username,twitter_password,twitter_message)
    try:
        song.file.path.encode(enc)
    except:
        filepath = song.file.path.encode(fsenc, 'ignore')
    return song.file.path.encode(fsenc, 'ignore')

def isGoodSong(song):
    if song.is_locked() :
        return False
    global Weight
    if song.rating_votes < 5: # Not voted or few votes
        C = 'N'
    else:
        C = int(round(song.rating))
    if Weight[C] >= songweight[C]:
        Weight[C] = 0
        return True
    #print "Debug : C = %s, Weight[C] = %s, songweight[C] = %s" % (C, Weight[C], songweight[C])
    for X in Weight.keys():
        Weight[X] += 1
    return False

def getRandom():
    songs = Song.active.count()
    rand = random.randint(0,songs-1)
    song = Song.active.all()[rand]
    C = 0
    # Try to find a good song that is not locked. Will try up to 10 times.
    while not isGoodSong(song) and C < 10:
       print "Random ", C
       rand = random.randint(0,songs-1)
       song = Song.active.all()[rand]
       C += 1
    #Q = Queue(song=song, played = True, requested_by=djUser)
    #Q.save()
    Q = common.queue_song(song, djUser, False, True)
    common.play_queued(Q)
    return song

def JingleTime():
    global jt_count
    global jt_timelast
    if jt_timelast + jt_min < datetime.datetime.now():
        if jt_count >= 10 or jt_max + jt_timelast < datetime.datetime.now():
            jt_count = 0
            jt_timelast = datetime.datetime.now()
            S = Song.objects.filter(status='J').order_by('?')[0]
            print "JingleTime!"
            return S
    jt_count += 1
    return False

def findQueued():
    songs = Queue.objects.filter(played=False, playtime__lte = datetime.datetime.now()).order_by('-priority', 'id')
    if not songs: # Since OR queries have been problematic on production server earlier, we do this hack..
        songs = Queue.objects.filter(played=False, playtime = None).order_by('-priority', 'id')
    if settings.PLAY_JINGLES:
        jingle = JingleTime()
        if jingle:
            return jingle
    if songs:
        song = songs[0]
        common.play_queued(song)
        return song.song
    else:
        return getRandom()

# This function, if defined, returns the string you'd like used
# as metadata (ie for title streaming) for the current song. You may
# return null to indicate that the file comment should be used.
def ices_get_metadata ():
        #return 'Artist - Title (Label, Year)'
        return meta.encode(enc, 'replace')

# function to update twitter with currently playing song
def tweet(user, password, message):
	if len(message) < 140:
		url = 'http://twitter.com/statuses/update.xml'
		curl = 'curl -s -u %s:%s -d status="%s" %s' % (user,password,message,url)
		pipe=popen(curl, 'r')
