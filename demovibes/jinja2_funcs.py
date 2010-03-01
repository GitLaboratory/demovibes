from django.template.defaulttags import URLNode
from django.conf import settings
from jinja2.filters import contextfilter
from django.utils import translation
from django.template import defaultfilters
from jinja2 import Markup, escape, environmentfilter

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

def mksafe(arg):
    """
    Force escaping of html
    
    First turn it into a Markup() type with escape, then force it into unicode again,
    so other modifications to the string won't be automatically escaped.
    """
    result = escape(arg)
    result = unicode(result)
    return result


#dict of filters
FILTERS = {
    'time': defaultfilters.time,
    'date': defaultfilters.date,
    'floatformat': defaultfilters.floatformat,
    'smileys': dv_extend.smileys,
    'oneliner_mediaparse': dv_extend.oneliner_mediaparse,
    'bbcode_oneliner': dv_extend.bbcode_oneliner,
    'dv_urlize': dv_extend.dv_urlize,
    'mksafe': mksafe,
}

# Dictionary over globally avaliable variables and functions
GLOBALS = {
    'url': url,
    'dummy': dummy,
    'dv': dv_extend,
}
