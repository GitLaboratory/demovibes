import uwsgi
import bottle
#from bottle import route, default_app, request, post, get
import pickle

try:
    import local_config
    allowed_ips = local_config.allowed_ips
    debug = getattr(local_config, "debug", False)
except:
    debug = False
    allowed_ips = ["127.0.0.1"]

bottle.debug(debug)


event = None

@bottle.post('/demovibes/ajax/monitor/new/')
@bottle.route('/demovibes/ajax/monitor/new/')
def http_event_receiver():
    ip = bottle.request.environ.get('REMOTE_ADDR')
    if ip not in allowed_ips:
        return ip
    data = bottle.request.forms.get('data')
    data = pickle.loads(data)
    event_receiver(data, 0)   
    return "OK"

def event_receiver(obj, id):
    global event
    event = obj
    uwsgi.green_unpause_all()

uwsgi.message_manager_marshal = event_receiver

@bottle.get('/demovibes/ajax/monitor/:id#[0-9]+#/')
def handler(id):
    id = int(id)
    yield ""

    if not event or event[1] <= id:
        uwsgi.green_pause(60)
    levent = [x[1] for x in event[0] if x[0] >= id]
    levent = set(levent)
    yield "\n".join(levent)
    yield "\n!%s" % event[1]
        
application = bottle.default_app()
