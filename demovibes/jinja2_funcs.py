from django.template.defaulttags import URLNode
from django.conf import settings
from jinja2.filters import contextfilter
from django.utils import translation

from webview.templatetags import dv_extend

def dummy(dummystuff):
    return "Dummy for %s" % dummystuff

def url(view_name, *args, **kwargs):
    from django.core.urlresolvers import reverse, NoReverseMatch
    try:
        return reverse(view_name, args=args, kwargs=kwargs)
    except NoReverseMatch:
        try:
            project_name = settings.SETTINGS_MODULE.split('.')[0]
            return reverse(project_name + '.' + view_name,
                           args=args, kwargs=kwargs)
        except NoReverseMatch:
            return ''

def nbspize(text):
    import re
    return re.sub('\s','&nbsp;',text.strip())

def get_lang():
    return translation.get_language()

def timesince(date):
    from django.utils.timesince import timesince
    return timesince(date)

def timeuntil(date):
    from django.utils.timesince import timesince
    from datetime import datetime
    return timesince(datetime.now(),datetime(date.year, date.month, date.day))

def truncate(text,arg):
    import re
    from django.utils.encoding import force_unicode
    text = force_unicode(text)
    matches = re.match('(\d+)([cw]{1})',arg)
    if not matches:
        return text
    count = int(matches.group(1))
    type = matches.group(2)

    if type == 'c':
        if count > len(text):
            return text
        else:
            return text[:count] + '&hellip;'
    elif type == 'w':
        arr = text.strip().split()
        if count > len(arr):
            return ' '.join(arr)
        else:
            return ' '.join(arr[:count]) + '&hellip;'


# Dictionary over globally avaliable variables and functions
GLOBALS = {
    'url': url,
    'dummy': dummy,
    'dv': dv_extend,
}