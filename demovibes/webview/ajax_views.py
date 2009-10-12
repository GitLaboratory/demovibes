from demovibes.webview.models import *
from demovibes.webview.common import *
from django.core.urlresolvers import reverse
from django.http import HttpResponseRedirect
from django.http import HttpResponse
from django.contrib.auth.decorators import login_required
from django.shortcuts import render_to_response
from django.template import RequestContext
from django.conf import settings
from django.db.models import Q
import time, datetime

def monitor(request, event_id):
    if request.user.is_authenticated():
        P = get_profile(request.user)
        P.last_activity = datetime.datetime.now()
        P.save()
    for x in range(120):
        R = AjaxEvent.objects.filter(id__gt=event_id).order_by('id')
        if R:
            entries = list()
            for event in R:
                if event.user == None or event.user == request.user:
                    if not str(event.event) in entries:
                        entries.append(str(event.event))
            ajaxid = R.order_by('-id')[0].id
            return render_to_response('webview/js/manager.html', \
                { 'events' : entries, 'id' : ajaxid },  context_instance=RequestContext(request))
        time.sleep(1)
    return HttpResponse("")

#This might need to be uncached later on, if per-user info is sent.
def nowplaying(request):
    song = Queue.objects.select_related(depth=2).filter(played=True).order_by('-time_played')[0]
    return render_to_response('webview/js/now_playing.html', { 'now_playing' : song },  context_instance=RequestContext(request))

def history(request):
    return HttpResponse(get_history())

def queue(request):
    return HttpResponse(get_queue())

def oneliner_submit(request):
    if not request.user.is_authenticated():
        return HttpResponse("NoAuth")
    message = request.POST['Line'].strip()
    if message != "":
        Oneliner.objects.create(user = request.user, message = message)
        request.path = reverse('dv-ax-oneliner')
        f = get_oneliner(True)
        AjaxEvent.objects.create(event='oneliner')
    return HttpResponse("OK")

def oneliner(request):
    return HttpResponse(get_oneliner())

def songupdate(request, song_id):
    song = Song.objects.get(id=song_id)
    return render_to_response('webview/js/generic.html', { 
        'song' : song,
        'event' : "a_queue_%i" % song.id,
        'template' : 'webview/t/songlist_span.html',
        },  context_instance=RequestContext(request))

def words(request, prefix): 
    extrawords=['boobies','boobietrap','nectarine']; 
    words= [a.username+"" for a in User.objects.filter(username__istartswith=prefix)];
    words.extend([a.handle+"" for a in Artist.objects.filter(handle__istartswith=prefix)]);
    words.extend([a.name+"" for a in Artist.objects.filter(name__istartswith=prefix)]);
    words.extend([a.name+"" for a in Group.objects.filter(name__istartswith=prefix)]);
    words.extend([a for a in extrawords if a.lower().startswith(prefix.lower())]);
    return HttpResponse(",".join(words));
