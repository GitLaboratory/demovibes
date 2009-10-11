from demovibes.webview.models import *
from demovibes.webview.forms import *
from demovibes.webview.common import get_profile, get_latest_event

from django import forms
from django.http import HttpResponseRedirect, HttpResponseNotFound, HttpResponseBadRequest, HttpResponse

from django.contrib.auth.decorators import login_required, permission_required
from django.contrib.auth import logout
from django.shortcuts import render_to_response, get_object_or_404
from django.template import RequestContext, TemplateDoesNotExist
from django.conf import settings
from django.views.generic.simple import direct_to_template

from django.core.urlresolvers import reverse
from django.core.paginator import Paginator, EmptyPage, InvalidPage
from django.core.files.base import File
from django.core.cache import cache

from random import choice

import time
import mad
import mimetypes
import os
# Create your views here.

def about_pages(request, page):
    try:
        return direct_to_template(request, template="about/%s.html" % page)
    except TemplateDoesNotExist:
        return HttpResponseNotFound()

@login_required
def inbox(request):
    pms = request.GET.get('type','')
    delete = request.GET.get('delete','')
    if delete:
        try:
            delpm = int(delete)
            pm = PrivateMessage.objects.get(pk = delpm, to = request.user)
        except:
            return HttpResponseNotFound()
        pm.visible = False
        pm.save()
    if pms == "sent":
        mails = PrivateMessage.objects.filter(sender = request.user, visible = True)
    else:
        pms = "received" #to remove injects
        mails = PrivateMessage.objects.filter(to = request.user, visible = True)
    return render_to_response('webview/inbox.html', {'mails' : mails, 'pms': pms}, context_instance=RequestContext(request))


@login_required
def read_pm(request, pm_id):
    pm = get_object_or_404(PrivateMessage, id = pm_id)
    if pm.to == request.user:
        pm.unread = False
        pm.save()
        return render_to_response('webview/view_pm.html', {'pm' : pm}, context_instance=RequestContext(request))
    if pm.sender == request.user:
        return render_to_response('webview/view_pm.html', {'pm' : pm}, context_instance=RequestContext(request))
    return HttpResponseRedirect(reverse('dv-inbox'))

@login_required
def send_pm(request):
    if request.method == 'POST':
        form = PmForm(request.POST)
        if form.is_valid():
            F = form.save(commit=False)
            F.sender=request.user
            F.save()
            return HttpResponseRedirect(reverse('dv-inbox'))
    else:
        title = request.GET.get('title', "")
        to = request.GET.get('to', "")
        try:
            U = User.objects.get(username=to)
        except:
            U = None
        form = PmForm(initial= {'to': U, 'subject' : title})
    return render_to_response('webview/pm_send.html', {'form' : form}, context_instance=RequestContext(request))

@login_required
def addqueue(request, song_id): # XXX Fix to POST
    """
    Add a song to the playing queue.
    """
    try:
        song = Song.objects.get(id=song_id)
    except:
        return HttpResponseNotFound()
    song.queue_by(request.user)
    return direct_to_template(request, template = "webview/song_queued.html")

@login_required
def addcomment(request, song_id):
    """
    Add a comment to a song.
    """
    if request.method == 'POST':
        comment = request.POST['Comment'].strip()
        song = get_object_or_404(Song, id=song_id)
        if comment:
           form = SongComment(comment = request.POST['Comment'], song = song, user = request.user)
           form.save()
    return HttpResponseRedirect(song.get_absolute_url())

def site_about(request):
    """
    Support for a generic 'About' function
    """
    return render_to_response('webview/site-about.html', { }, context_instance=RequestContext(request))

def site_faq(request):
    """
    Support for a generic FAQ function
    """
    return render_to_response('webview/site-faq.html', { }, context_instance=RequestContext(request))

def site_links(request):
    """
    Support for a generic links
    """
    return render_to_response('webview/site-links.html', { }, context_instance=RequestContext(request))

def list_queue(request):
    """
    Display the current song, the next songs in queue, and the latest 20 songs in history.
    """
    try :
        now_playing = Queue.objects.select_related(depth=2).filter(played=True).order_by('-time_played')[0]
    except IndexError:
        now_playing = ""
    history = Queue.objects.select_related(depth=2).filter(played=True).order_by('-time_played')[1:21]
    queue = Queue.objects.select_related(depth=2).filter(played=False).order_by('id')
    return render_to_response('webview/queue_list.html', \
        {'now_playing': now_playing, 'history': history, 'queue': queue}\
        , context_instance=RequestContext(request))
    
def list_song(request, song_id):
    song = get_object_or_404(Song, id=song_id)
    
    # We can now get any compilation data that this song is a part of
    comps = Compilation.objects.filter(songs__id = song.id)
    
    # Has this song been remixed?
    remix = Song.objects.filter(remix_of_id = song.id)
    
    return render_to_response('webview/song_detail.html', \
        { 'object' : song, 'vote_range': [1, 2, 3, 4, 5], 'comps' : comps, 'remix' : remix }\
        , context_instance=RequestContext(request))

def view_user_favs(request, user):
    U = get_object_or_404(User, username = user)
    profile = get_profile(U)
    if not profile.viewable_by(request.user):
        return render_to_response('base/error.html', { 'error' : "Sorry, you're not allowed to see this" }, context_instance=RequestContext(request))
    favorites = Favorite.objects.filter(user = U)
    return render_to_response('webview/user_favorites.html', \
        {'favorites' : favorites, 'favuser' : U}, \
        context_instance=RequestContext(request))

@login_required
def my_profile(request):
    """
    Display the logged in user's profile.
    """
    user = request.user
    profile = get_profile(user)
    if request.method == 'POST':
        form = ProfileForm(request.POST, request.FILES, instance = profile)
        if form.is_valid():
            form.save()
            return HttpResponseRedirect(reverse('dv-my_profile')) # To hinder re-post on refresh
    else:
        form = ProfileForm(instance=profile)
    return render_to_response('webview/my_profile.html', {'profile' : profile, 'form' : form}, context_instance=RequestContext(request))


def view_profile(request, user):
    """
    Display a user's profile.
    """
    ProfileUser = get_object_or_404(User,username = user)
    profile = get_profile(User.objects.get(username=user))
    if profile.viewable_by(request.user):
        return render_to_response('webview/view_profile.html', \
            {'profile' : profile}, \
            context_instance=RequestContext(request))
    return render_to_response('base/error.html', { 'error' : "Sorry, you're not allowed to see this" }, context_instance=RequestContext(request))

def search(request):
    """
    Return the first 40 matches of songs, artists and groups.
    """
    if request.method == 'POST' and "Search" in request.POST:
        searchterm = request.POST['Search']
        result_limit = getattr(settings, 'SEARCH_LIMIT', 40)
        if settings.USE_FULLTEXT_SEARCH == True:
            users = User.objects.filter(username__search = searchterm)[:result_limit]
            songs = Song.objects.select_related(depth=1).filter(title__search = searchterm)[:result_limit]
            artists = Artist.objects.filter(handle__search = searchterm)|Artist.objects.filter(name__search = searchterm)[:result_limit]
            groups = Group.objects.filter(name__search = searchterm)[:result_limit]
            compilations = Compilation.objects.filter(name__search = searchterm)[:result_limit]
        else:
            users = User.objects.filter(username__icontains = searchterm)[:result_limit]
            songs = Song.objects.select_related(depth=1).filter(title__icontains = searchterm)[:result_limit]
            artists = Artist.objects.filter(handle__icontains = searchterm)|Artist.objects.filter(name__icontains = searchterm)[:result_limit]
            groups = Group.objects.filter(name__icontains = searchterm)[:result_limit]
            compilations = Compilation.objects.filter(name__icontains = searchterm)[:result_limit]
        return render_to_response('webview/search.html', \
            { 'songs' : songs, 'artists' : artists, 'groups' : groups, 'users' : users, 'compilations' : compilations }, \
            context_instance=RequestContext(request))
    return render_to_response('webview/search.html', {}, context_instance=RequestContext(request))

def show_approvals(request):
    """
    Shows the most recently approved songs in it's own window
    """
    
    result_limit = getattr(settings, 'UPLOADED_SONG_COUNT', 150)
    songs = SongApprovals.objects.order_by('-approved')[:result_limit]
    
    return render_to_response('webview/recent_approvals.html', { 'songs': songs , 'settings' : settings }, context_instance=RequestContext(request))

def list_artists(request, letter):
    """
    List artists that start with a certain letter.
    """
    if not letter in alphalist or letter == '-':
        letter = '#'
    artists = Artist.objects.filter(startswith=letter)
    paginator = Paginator(artists, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        artistic = paginator.page(page)
    except (EmptyPage, InvalidPage):
        artistic = paginator.page(paginator.num_pages)
    return render_to_response('webview/artist_list.html', \
        {'object_list' : artistic.object_list, 'page_range' : paginator.page_range, \
            'page' : page, 'letter' : letter, 'al': alphalist}, \
        context_instance=RequestContext(request))

def list_groups(request, letter):
    """
    List groups that start with a certain letter.
    """
    if not letter in alphalist or letter == '-':
        letter = '#'
    groups = Group.objects.filter(startswith=letter).filter(status="A")
    paginator = Paginator(groups, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        groupic = paginator.page(page)
    except (EmptyPage, InvalidPage):
        groupic = paginator.page(paginator.num_pages)
    return render_to_response('webview/group_list.html', \
        {'object_list' : groupic.object_list, 'page_range' : paginator.page_range, \
            'page' : page, 'letter' : letter, 'al': alphalist}, \
        context_instance=RequestContext(request))
    
def list_labels(request, letter):
    """
    List labels that start with a certain letter.
    """
    if not letter in alphalist or letter == '-':
        letter = '#'
    labels = Label.objects.filter(startswith=letter).filter(status="A")
    paginator = Paginator(labels, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        labelic = paginator.page(page)
    except (EmptyPage, InvalidPage):
        labelic = paginator.page(paginator.num_pages)
    return render_to_response('webview/label_list.html', \
        {'object_list' : labelic.object_list, 'page_range' : paginator.page_range, \
            'page' : page, 'letter' : letter, 'al': alphalist}, \
        context_instance=RequestContext(request))

def list_compilations(request, letter):
    """
    List compilations that start with a certain letter.
    """
    if not letter in alphalist or letter == '-':
        letter = '#'
    compilations = Compilation.objects.filter(startswith=letter)
    paginator = Paginator(compilations, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        complist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        complist = paginator.page(paginator.num_pages)
    return render_to_response('webview/compilation_list.html', \
        {'object_list' : complist.object_list, 'page_range' : paginator.page_range, \
            'page' : page, 'letter' : letter, 'al': alphalist}, \
        context_instance=RequestContext(request))

@login_required
def log_out(request):
    """
    Show a user a form, and then logs user out if a form is sent in to that address.
    """
    if request.method == 'POST':
        logout(request)
        return HttpResponseRedirect("/")
    return render_to_response('webview/logout.html', {}, context_instance=RequestContext(request))

def list_songs(request, letter):
    """
    List songs that start with a certain letter.
    """
    if not letter in alphalist or letter == '-':
        letter = '#'
    songs = Song.objects.select_related(depth=1).filter(startswith=letter)
    paginator = Paginator(songs, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        songlist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        songlist = paginator.page(paginator.num_pages)
    return render_to_response('webview/song_list.html', \
        {'object_list' : songlist.object_list, 'page_range' : paginator.page_range, \
         'page' : page, 'letter' : letter, 'al' : alphalist}, \
        context_instance = RequestContext(request))

def list_song_history(request, song_id):
    """
    List the queue history belonging to a song
    """
    song = get_object_or_404(Song, id=song_id)
    history = song.queue_set.all()
    paginator = Paginator(history, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        historylist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        historylist = paginator.page(paginator.num_pages)
    return render_to_response('webview/song_history.html', \
        { 'requests' : historylist.object_list, 'song' : song, 'page' : page, 'page_range' : paginator.page_range  },\
        context_instance = RequestContext(request))

def list_song_votes(request, song_id):
    """
    List the votes belonging to a song
    """
    song = get_object_or_404(Song, id=song_id)
    votes = song.songvote_set.all()
    paginator = Paginator(votes, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        votelist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        votelist = paginator.page(paginator.num_pages)
    return render_to_response('webview/song_votes.html', \
        { 'votelist' : votelist.object_list, 'song' : song, 'page' : page, 'page_range' : paginator.page_range  },\
        context_instance = RequestContext(request))

def list_song_comments(request, song_id):
    """
    List the comments belonging to a song
    """
    song = get_object_or_404(Song, id=song_id)
    comments = song.songcomment_set.all()
    paginator = Paginator(comments, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        commentlist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        commentlist = paginator.page(paginator.num_pages)
    return render_to_response('webview/song_comments.html', \
        { 'commentlist' : commentlist.object_list, 'song' : song, 'page' : page, 'page_range' : paginator.page_range  },\
        context_instance = RequestContext(request))

def view_compilation(request, comp_id):
    """
    Try to view a compilation entry.
    """
    comp = get_object_or_404(Compilation, id=comp_id) # Find it, or return a 404 error
    return render_to_response('webview/compilation.html', { 'comp' : comp, 'user' : request.user }, context_instance=RequestContext(request))

@login_required
def add_favorite(request, id): # XXX Fix to POST
    """
    Add a song to the user's favorite. Takes one argument, song id.
    """
    user = request.user
    song = get_object_or_404(Song, id=id)
    Q = Favorite.objects.filter(user = user, song = song)
    if not Q: # Does the user already have this as favorite?
        f = Favorite(user=user, song=song)
        f.save()
    return HttpResponseRedirect(reverse('dv-favorites'))

def oneliner(request):
    oneliner = Oneliner.objects.select_related(depth=1).order_by('-id')[:20]
    return render_to_response('webview/oneliner.html', {'oneliner' : oneliner}, \
        context_instance=RequestContext(request))

@login_required
def oneliner_submit(request):
    """
    Add a text line to the oneliner.
    Returns user to referrer position, or to /
    """
    try:
        message =  request.POST['Line'].strip()
        if message != "":
            Oneliner.objects.create(user = request.user, message = message)
            AjaxEvent.objects.create(event='oneliner')
    except:
        pass
    try:
        refer = request.META['HTTP_REFERER']
        return HttpResponseRedirect(refer)
    except:
        return HttpResponseRedirect("/")

@login_required
def list_favorites(request):
    """
    Display a user's favorites.
    """
    user = request.user
    songs = Favorite.objects.filter(user=user)
    paginator = Paginator(songs, settings.PAGINATE)
    page = int(request.GET.get('page', '1'))
    try:
        songlist = paginator.page(page)
    except (EmptyPage, InvalidPage):
        songlist = paginator.page(paginator.num_pages)
    return render_to_response('webview/favorites.html', \
      {'songs': songlist.object_list, 'page' : page, 'page_range' : paginator.page_range}, \
      context_instance=RequestContext(request))

@login_required
def del_favorite(request, id): # XXX Fix to POST
    """
    Removes a favorite from the user's list.
    """
    S = Song.objects.get(id=id)
    Q = Favorite.objects.filter(user = request.user, song = S)
    if Q:
        Q[0].delete()
    return HttpResponseRedirect(reverse('dv-favorites'))


    
@login_required
def upload_song(request, artist_id):
    artist = get_object_or_404(Artist, id=artist_id)
    
    # Quick test to see if the artist is currently active. If not, bounce
    # To the current queue!
    if artist.status != 'A':
        return HttpResponseRedirect(reverse('dv-queue'))
        
    if request.method == 'POST':
        if artist.link_to_user == request.user:
            status = 'A'
        else:
            status = 'U'
        a = Song(uploader = request.user, status = status)
        form = UploadForm(request.POST, request.FILES, instance = a)
        if form.is_valid():
            new_song = form.save(commit=False)
            new_song.save()
            new_song.artists.add(artist)
            form.save_m2m()
            return HttpResponseRedirect(new_song.get_absolute_url())
    else:
        form = UploadForm()
    return render_to_response('webview/upload.html', \
        {'form' : form, 'artist' : artist }, \
        context_instance=RequestContext(request))

@permission_required('webview.change_song')
def activate_upload(request):
    if "song" in request.GET and "status" in request.GET:
        songid = int(request.GET['song'])
        status = request.GET['status']
        song = Song.objects.get(id=songid)

        if status == 'A':
            stat = "Accepted"
            song.status = "A"
        if status == 'R':
            stat = "Rejected"
            song.status = 'R'

        # This used to be propriatary, it is now a template. AAK
        mail_tpl = loader.get_template('webview/email/song_approval.txt')
        c = Context({
                'songid' : songid,
                'site' : Site.objects.get_current(),
                'stat' : stat,
        })
        song.save()

        # Only add if song is approved!
        if(status == 'A'):
            Q = SongApprovals(song=song, approved_by=request.user, uploaded_by=song.uploader)
            Q.save()
        
        if song.uploader.get_profile().pm_accepted_upload and status == 'A' or status == 'R':
            PrivateMessage.objects.create(sender = request.user, to = song.uploader,\
             message = mail_tpl.render(c), subject = "Song Upload Status Changed To: %s" % stat)
    songs = Song.objects.filter(status = "U").order_by('added')
    return render_to_response('webview/uploaded_songs.html', {'songs' : songs}, context_instance=RequestContext(request))

@login_required
def create_artist(request):
    """
    Simple form to allow registereed users to create a new artist entry.
    """
    if request.method == 'POST':
        a = Artist(created_by = request.user, status = 'U')
        form = CreateArtistForm(request.POST, request.FILES, instance = a)
        if form.is_valid():
            new_artist = form.save(commit=False)
            new_artist.save()
            form.save_m2m()
            return HttpResponseRedirect(new_artist.get_absolute_url())
    else:
        form = CreateArtistForm()
    return render_to_response('webview/create_artist.html', \
        {'form' : form }, \
        context_instance=RequestContext(request))

@permission_required('webview.change_artist')
def activate_artists(request):
    """
    Shows the most recently added artists who have a 'U' status in their upload marker
    """
    if "artist" in request.GET and "status" in request.GET:
        artistid = int(request.GET['artist'])
        status = request.GET['status']
        artist = Artist.objects.get(id=artistid)

        if status == 'A':
            stat = "Accepted"
            artist.status = "A"
        if status == 'R':
            stat = "Rejected"
            artist.status = 'R'

        # Prepare a mail template to inform user of the status of their request
        mail_tpl = loader.get_template('webview/email/artist_approval.txt')
        c = Context({
                'artist' : artist,
                'site' : Site.objects.get_current(),
                'stat' : stat,
        })
        artist.save()
        
        # Send the email to inform the user of their request status
        PrivateMessage.objects.create(sender = request.user, to = artist.created_by,\
         message = mail_tpl.render(c), subject = "Artist Request Status Changed To: %s" % stat)

    artists =Artist.objects.filter(status = "U").order_by('last_updated')
    return render_to_response('webview/pending_artists.html', { 'artists': artists }, context_instance=RequestContext(request))

@login_required
def create_group(request):
    """
    Simple form to allow registereed users to create a new group entry.
    """
    if request.method == 'POST':
        g = Group(created_by = request.user, status = 'U')
        form = CreateGroupForm(request.POST, request.FILES, instance = g)
        if form.is_valid():
            new_group = form.save(commit=False)
            new_group.save()
            form.save_m2m()
            return HttpResponseRedirect(new_group.get_absolute_url())
    else:
        form = CreateGroupForm()
    return render_to_response('webview/create_group.html', \
        {'form' : form }, \
        context_instance=RequestContext(request))

@permission_required('webview.change_group')
def activate_groups(request):
    """
    Shows the most recently added groups who have a 'U' status in their upload marker
    """
    if "group" in request.GET and "status" in request.GET:
        groupid = int(request.GET['group'])
        status = request.GET['status']
        group = Group.objects.get(id=groupid)

        if status == 'A':
            stat = "Accepted"
            group.status = "A"
        if status == 'R':
            stat = "Rejected"
            group.status = 'R'

        # Prepare a mail template to inform user of the status of their request
        mail_tpl = loader.get_template('webview/email/group_approval.txt')
        c = Context({
                'group' : group,
                'site' : Site.objects.get_current(),
                'stat' : stat,
        })
        group.save()
        
        # Send the email to inform the user of their request status
        PrivateMessage.objects.create(sender = request.user, to = group.created_by,\
         message = mail_tpl.render(c), subject = "Group Request Status Changed To: %s" % stat)

    groups =Group.objects.filter(status = "U").order_by('last_updated')
    return render_to_response('webview/pending_groups.html', { 'groups': groups }, context_instance=RequestContext(request))

@login_required
def create_label(request):
    """
    Simple form to allow registereed users to create a new label entry.
    """
    if request.method == 'POST':
        l = Label(created_by = request.user, status = 'U')
        form = CreateLabelForm(request.POST, request.FILES, instance = l)
        if form.is_valid():
            new_label = form.save(commit=False)
            new_label.save()
            form.save_m2m()
            return HttpResponseRedirect(new_label.get_absolute_url())
    else:
        form = CreateLabelForm()
    return render_to_response('webview/create_label.html', \
        {'form' : form }, \
        context_instance=RequestContext(request))
    
@permission_required('webview.change_label')
def activate_labels(request):
    """
    Shows the most recently added labels who have a 'U' status in their upload marker
    """
    if "label" in request.GET and "status" in request.GET:
        labelid = int(request.GET['label'])
        status = request.GET['status']
        this_label = Label.objects.get(id=labelid)

        if status == 'A':
            stat = "Accepted"
            this_label.status = "A"
        if status == 'R':
            stat = "Rejected"
            this_label.status = 'R'

        # Prepare a mail template to inform user of the status of their request
        mail_tpl = loader.get_template('webview/email/label_approval.txt')
        c = Context({
                'label' : this_label,
                'site' : Site.objects.get_current(),
                'stat' : stat,
        })
        this_label.save()
        
        # Send the email to inform the user of their request status
        PrivateMessage.objects.create(sender = request.user, to = this_label.created_by,\
         message = mail_tpl.render(c), subject = "Label Request Status Changed To: %s" % stat)

    labels = Label.objects.filter(status = "U").order_by('last_updated')
    return render_to_response('webview/pending_labels.html', { 'labels': labels }, context_instance=RequestContext(request))

    
def save_flash(request):
    # verify ticket + user
    ticket_id = request.GET['ticket']
    ticket = get_object_or_404(UploadTicket, ticket = ticket_id)
    if ticket.user != request.user:
        return HttpResponseBadRequest("")
    # save file to /tmp/
    tempfile = "/tmp/djutmp-%s" % ticket_id
    f = request.FILES['file']
    destination = open(tempfile, 'wb+')
    for chunk in f.chunks():
        destination.write(chunk)
    destination.close()
    # update ticket
    ticket.tempfile = tempfile
    ticket.filename = os.path.basename(f.name)
    ticket.save()
    # return "ok" to the flash app
    return HttpResponse("OK")


def users_online(request):
    timefrom = datetime.datetime.now() - datetime.timedelta(minutes=10)
    userlist = Userprofile.objects.filter(last_activity__gt=timefrom)
    return render_to_response('webview/online_users.html', {'userlist' : userlist}, context_instance=RequestContext(request))
    
@login_required
def upload_flash(request):
    entropy = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"
    ticket_id = ""
    for x in range(20):
        ticket_id += choice(entropy)
    if request.method == 'POST':
        ticket_id = request.POST['ticket']
        ticket = get_object_or_404(UploadTicket, ticket = ticket_id)
        if ticket.user != request.user or ticket.tempfile == "":
            HttpResponse("Invalid ticket!")
        a = Song(uploader = request.user, status = 'U')
        form = FlashUploadForm(request.POST, instance = a)
        if form.is_valid():
            F = File(open(ticket.tempfile, 'rb+'))
            new_song = form.save(commit=False)
            new_song.file.save(ticket.filename, F)
            new_song.save()
            new_song.artists.add(artist)
            form.save_m2m()
            return HttpResponseRedirect(new_song.get_absolute_url())
    else:
        #Not a post, generate new ticket
        ticket = Ticket(ticket=ticket_id, user = request.user)
        ticket.save()
        form = FlashUploadForm()
    return render_to_response('webview/flash_upload.html', \
        {'ticket' : ticket_id, 'form': form}, \
        context_instance=RequestContext(request))

@login_required
def set_rating_autovote(request, song_id, user_rating):
    """
    Set a user's rating on a song. From 0 to 5
    """
    int_vote = int(user_rating)
    if int_vote <= 5 and int_vote >= 0:
        S = Song.objects.get(id = song_id)
        S.set_vote(int_vote, request.user)
        AjaxEvent.objects.create(event="nowplaying")

        # Successful vote placed. 
        try:
            refer = request.META['HTTP_REFERER']
            return HttpResponseRedirect(refer)
        except:
            return HttpResponseRedirect("/")

    # If the user tries any funny business, we redirect to the queue. No messing!
    return HttpResponseRedirect(reverse("dv-queue"))

@login_required
def set_rating(request, song_id):
    """
    Set a user's rating on a song. From 0 to 5
    """
    if request.method == 'POST':
        try:
            R = int(request.POST['Rating'])
        except:
             return HttpResponseRedirect(reverse('dv-song', args=[song_id]))
        if R <= 5 and R >= 0:
            S = Song.objects.get(id = song_id)
            S.set_vote(R, request.user)
    return HttpResponseRedirect(S.get_absolute_url())
