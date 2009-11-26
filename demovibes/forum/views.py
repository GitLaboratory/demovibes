"""
All forum logic is kept here - displaying lists of forums, threads 
and posts, adding new threads, and adding replies.
"""

from forum.models import Forum,Thread,Post,Subscription
from forum.forms import ThreadForm, ReplyForm, EditForm
from datetime import datetime
from django.shortcuts import get_object_or_404, render_to_response
from django.http import Http404, HttpResponse, HttpResponseRedirect, HttpResponseServerError, HttpResponseForbidden
from django.template import RequestContext, Context, loader
from django import forms
from django.core.mail import EmailMessage
from django.conf import settings
from django.template.defaultfilters import striptags, wordwrap
from django.contrib.sites.models import Site
from django.core.urlresolvers import reverse
from django.core.paginator import Paginator, EmptyPage, InvalidPage

def forum_email_notification(post):
    try:
        mail_subject = settings.FORUM_MAIL_PREFIX 
    except AttributeError:
        mail_subject = '[Forum]'
    try:
        mail_from = settings.FORUM_MAIL_FROM
    except AttributeError:
        mail_from = settings.DEFAULT_FROM_EMAIL
    mail_tpl = loader.get_template('forum/notify.txt')
    c = Context({
        'body': wordwrap(striptags(post.body), 72),
        'site' : Site.objects.get_current(),
        'thread': post.thread,
        })
    email = EmailMessage(
            subject=mail_subject+' '+striptags(post.thread.title),
            body=mail_tpl.render(c),
            from_email=mail_from,
            to=[mail_from],
            bcc=[s.author.email for s in post.thread.subscription_set.all()])
    email.send(fail_silently=True)

def edit(request, post_id):
    P = get_object_or_404(Post, id=post_id)
    t = P.thread
    if request.user != P.author:
        return HttpResponseRedirect(t.get_absolute_url())
    if request.method == 'POST':
        edit_form = EditForm(request.POST, instance=P)
        if edit_form.is_valid():
            edit_form.save()
            return HttpResponseRedirect(t.get_absolute_url())
    else:
        edit_form = EditForm(instance=P)
    return render_to_response('forum/post_edit.html',
        RequestContext(request, {'edit_form' : edit_form}))

def forum(request, slug):
    """
    Displays a list of threads within a forum.
    Threads are sorted by their sticky flag, followed by their 
    most recent post.
    """
    f = get_object_or_404(Forum, slug=slug)

    # If the user is not authorized to view the thread, then redirect
    if f.is_private and request.user.is_staff != True:
         return HttpResponseRedirect('/forum')
    
    # Process new thread form if data was sent
    if request.method == 'POST':
        if not request.user.is_authenticated():
            return HttpResponseServerError()
        thread_form = ThreadForm(request.POST)
        if thread_form.is_valid():
            new_thread = thread_form.save(commit = False)
            new_thread.forum = f
            new_thread.save()
            Post.objects.create(thread=new_thread, author=request.user,
               	body=thread_form.cleaned_data['body'],
                time=datetime.now())
            if (thread_form.cleaned_data['subscribe'] == True):
                Subscription.objects.create(author=request.user,
                    thread=new_thread)
            return HttpResponseRedirect(new_thread.get_absolute_url())
    else:
        thread_form = ThreadForm()
        
    # Pagination
    t = f.thread_set.all()
    paginator = Paginator(t, settings.FORUM_PAGINATE)
    page = int(request.GET.get('page', 1))
    try:
        threads = paginator.page(page)
    except (EmptyPage, InvalidPage):
        threads = paginator.page(paginator.num_pages)

    return render_to_response('forum/thread_list.html',
        RequestContext(request, {
            'forum': f,
            'threads': threads.object_list,
            'page_range': paginator.page_range,
            'page': page,
            'thread_form': thread_form
        }))

def thread(request, thread):
    """
    Increments the viewed count on a thread then displays the 
    posts for that thread, in chronological order.
    """
    t = get_object_or_404(Thread, pk=thread)
    p = t.post_set.all().order_by('time')
    s = t.subscription_set.filter(author=request.user)

    # If the user is not authorized to view, we redirect them
    if t.forum.is_private and request.user.is_staff != True:
         return HttpResponseRedirect('/forum')

    # Process reply form if it was sent
    if (request.method == 'POST'):
        if not request.user.is_authenticated() or t.closed:
            return HttpResponseServerError()
        reply_form = ReplyForm(request.POST)
        if reply_form.is_valid():
            new_post = reply_form.save(commit = False)
            new_post.author = request.user
            new_post.thread = t
            new_post.time=datetime.now()
            new_post.save()
            # Change subscription
            if reply_form.cleaned_data['subscribe']:
                Subscription.objects.get_or_create(thread=t,
                    author=request.user)
            else:
                Subscription.objects.filter(thread=t, author=request.user).delete()
            # Send email
            forum_email_notification(new_post)
            return HttpResponseRedirect(new_post.get_absolute_url())
    else:
        reply_form = ReplyForm(initial={'subscribe': s})

    # Pagination
    paginator = Paginator(p, settings.FORUM_PAGINATE)
    page = int(request.GET.get('page', paginator.num_pages))
    try:
        posts = paginator.page(page)
    except (EmptyPage, InvalidPage):
        posts = paginator.page(paginator.num_pages)

    t.views += 1
    t.save()
    #{'object_list' : artistic.object_list, 'page_range' : paginator.page_range, 'page' : page, 'letter' : letter, 'al': alphalist}, \
    return render_to_response('forum/thread.html',
        RequestContext(request, {
            'forum': t.forum,
            'thread': t,
            'posts': posts.object_list,
	    'page_range': paginator.page_range,
            'page': page,
            'reply_form': reply_form
        }))

def updatesubs(request):
    """
    Allow users to update their subscriptions all in one shot.
    """
    if not request.user.is_authenticated():
        return HttpResponseForbidden(_('Sorry, you need to login.'))

    subs = Subscription.objects.filter(author=request.user)

    if request.POST:
        # remove the subscriptions that haven't been checked.
        post_keys = [k for k in request.POST.keys()]
        for s in subs:
            if not str(s.thread.id) in post_keys:
                s.delete()
        return HttpResponseRedirect(reverse('forum_subscriptions'))

    return render_to_response('forum/updatesubs.html',
        RequestContext(request, {
            'subs': subs,
        }))
       
