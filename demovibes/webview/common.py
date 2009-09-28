import time
from demovibes.webview.models import Userprofile
from django.conf import settings
from demovibes.webview.models import AjaxEvent
from django.core.cache import cache

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


# Not perfect, borks if I add () to decorator (or arguments..)
# Tried moving logic to call and def a wrapper there, but django somehow didn't like that
#
# Code will try to find an "event" value in the GET part of the url. If it can't find it,
# the current event number is collected from database.
class cache_output(object):
    
    def __init__(self, f):
        self.f = f
        self.n = f.__name__
        self.s = 60*10 # default cache time in seconds
        
    def __call__(self, *args, **kwargs):
        try:
            log_debug("Cache", "Starting caching function", 2)
            try:
                path = args[0].path
            except:
                log_debug("Cache","Error, no path")
                path = self.n
            try:
                event = args[0].GET['event']
            except: # no event get string found
                event = get_latest_event()
            key = "%s.%s" % (path, event)
            value = cache.get(key)
            if not value:
                log_debug("Cache", "Could not find cache for key '%s'" % key)
                value = self.f(*args, **kwargs)
                cache.set(key, value, self.s)
        except:
            log_debug("Cache", "Error, could not check cache!")
            value = self.f(*args, **kwargs)
        return value