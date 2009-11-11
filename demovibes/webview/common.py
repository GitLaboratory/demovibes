import time
from webview.models import Queue, Oneliner, Userprofile, Compilation, AjaxEvent, Song, User
from django.conf import settings
from django.core.cache import cache
from django.shortcuts import render_to_response
from django.template import Context, Template
from django.template.loader import get_template
from django.conf import settings
import logging
import datetime

def play_queued(queue):
    queue.song.times_played = queue.song.times_played + 1
    queue.song.save()
    queue.time_played=datetime.datetime.now()
    queue.played = True
    queue.save()
    temp = get_now_playing(True)
    temp = get_history(True)
    temp = get_queue(True)
    AjaxEvent.objects.create(event="queue")
    AjaxEvent.objects.create(event="history")
    AjaxEvent.objects.create(event="nowplaying")
    
    
# This function should both make cake, and eat it
def queue_song(song, user, event = True, force = False):
    sl = settings.SONG_LOCK_TIME
    Q = False
    time = datetime.timedelta(hours = sl['hours'], days = sl['days'], minutes = sl['minutes'])
    result = True
    if not force:
        Queue.objects.lock(Song, User, AjaxEvent)
        requests = Queue.objects.filter(played=False, requested_by = user).count()
        if requests >= settings.SONGS_IN_QUEUE:
            AjaxEvent.objects.create(event='eval:alert("You have reached your queue limit. Wait for the songs to play.");', user = user)
            result = False
        if result and song.is_locked():
            result = False
    if result:
        song.locked_until = datetime.datetime.now() + time
        song.save()
        Q = Queue(song=song, requested_by=user, played = False)
        Q.save()
    Queue.objects.unlock()
    if result:
        Q.eta = Q.get_eta()
        Q.save()
        AjaxEvent.objects.create(event='a_queue_%i' % song.id)
        if event:
            bla = get_queue(True) # generate new queue cached object
            AjaxEvent.objects.create(event='queue')
    return Q


def get_now_playing(create_new=True):
    logging.debug("Getting now playing")
    key = "nnowplaying" # Change to non-event type key later, when Queue access system have been fixed
    R = cache.get(key)
    if not R or create_new:
        try:
            songtype = Queue.objects.select_related(depth=3).filter(played=True).order_by('-time_played')[0]
            song = songtype.song
        except:
            return ""
        comps = Compilation.objects.filter(songs__id = song.id)
        T = get_template('webview/t/now_playing_song.html')
        C = Context({ 'now_playing' : songtype, 'comps' : comps })
        R = T.render(C)   
        cache.set(key, R, 300)
        logging.debug("Now playing generated")
    return R

def get_history(create_new=False):
    key = "nhistory" # Change to non-event type key later, when Queue access system have been fixed
    logging.debug("Getting history cache")
    R = cache.get(key)
    if not R or create_new:
        logging.info("No existing cache for history, making new one")
        history = Queue.objects.select_related(depth=3).filter(played=True).order_by('-time_played')[1:21]
        T = get_template('webview/js/history.html')
        C = Context({ 'history' : history })
        R = T.render(C)
        #R = render_to_response('webview/js/history.html', { 'history' : history })
        cache.set(key, R, 300)
        logging.debug("Cache generated")
    return R

def get_oneliner(create_new=False):
    key = "noneliner"
    logging.debug("Getting oneliner cache")
    R = cache.get(key)
    if not R or create_new:
        logging.info("No existing cache for oneliner, making new one")
        lines = getattr(settings, 'ONELINER', 10)
        oneliner = Oneliner.objects.select_related(depth=2).order_by('-id')[:lines]
        T = get_template('webview/js/oneliner.html')
        C = Context({ 'oneliner' : oneliner })
        R = T.render(C)
        cache.set(key, R, 600)
        logging.debug("Cache generated")
    return R

def get_queue(create_new=False):
    key = "nqueue" # Change to non-event type key later, when Queue access system have been fixed
    logging.debug("Getting cache for queue")
    R = cache.get(key)
    if not R or create_new:
        logging.info("No existing cache for queue, making new one")
        queue = Queue.objects.select_related(depth=2).filter(played=False).order_by('id')
        T = get_template('webview/js/queue.html')
        C = Context({ 'queue' : queue })
        R = T.render(C)
        #R = render_to_response('webview/js/queue.html', )
        cache.set(key, R, 300)
        logging.debug("Cache generated")
    return R

def get_profile(user):
    """
    Get a user's profile.

    Tries to get a user's profile, and create it if it doesn't exist.
    """
    try:
        profile = user.get_profile()
    except Userprofile.DoesNotExist:
        profile = Userprofile(user = user)
        profile.save()
    return profile

def get_latest_event():
    try:
        return AjaxEvent.objects.order_by('-id')[0].id
    except:
        return 0

def log_debug(area, text, level=1):
    settings_level = getattr(settings, 'DEBUGLEVEL', 0)
    settings_debug = getattr(settings, 'DEBUG')
    settings_file = getattr(settings, 'DEBUGFILE', "/tmp/django-error.log")
    if settings_debug and level <= settings_level:
        F = open(settings_file, "a")
        F.write("(%s) <%s:%s> %s\n" % (time.strftime("%d/%b/%Y %H:%M:%S", time.localtime()), area, level, text))
        F.close()


def get_event_key(key):
    event = get_latest_event()
    return "%sevent%s" % (key, event)
    
# Not perfect, borks if I add () to decorator (or arguments..)
# Tried moving logic to call and def a wrapper there, but django somehow didn't like that
#
# Code will try to find an "event" value in the GET part of the url. If it can't find it,
# the current event number is collected from database.
class cache_output(object):
    
    def __init__(self, f):
        self.f = f
        self.n = f.__name__
        self.s = 60*5 # default cache time in seconds
        
    def __call__(self, *args, **kwargs):
        try:
            try:
                path = args[0].path
            except:
                path = self.n
            try:
                event = args[0].GET['event']
            except: # no event get string found
                event = get_latest_event()
            key = "%s.%s" % (path, event)
            value = cache.get(key)
            if not value:
                value = self.f(*args, **kwargs)
                cache.set(key, value, self.s)
        except:
            value = self.f(*args, **kwargs)
        return value
