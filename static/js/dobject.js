var ajaxurl="/demovibes/ajax/";
var ajaxeventid=0; // updated later
var debug;
var ajaxmonitorrequest=false;

function ajaxencode(data) {
    return encodeURIComponent(data);
}

function ajaxhttprequest(mimetype) {
    var request=false;
    if (window.XMLHttpRequest) { // Mozilla, Safari,...
        request = new XMLHttpRequest();
        if (request.overrideMimeType) {
            request.overrideMimeType(mimetype); // set accordingly
        }
    } else if (window.ActiveXObject) { // IE
        try {
            request = new ActiveXObject("Msxml2.XMLHTTP");
        } catch (e) {
            try {
                request = new ActiveXObject("Microsoft.XMLHTTP");
            } catch (e) {}
        }
    }
    return request;
}

function ajaxget(url, callback, async) {
    var request = false;
    request=ajaxhttprequest('text/html');
    if (!request) {
        alert('Cannot create XMLHTTP instance');
        return false;
    }
    if (callback)
    request.onreadystatechange =  function(func) {
        return function() {
            if (request.readyState != 4)                
                return;
            callback(request);
        }(callback);
    }
    request.open('GET', url, async);
    request.send(null);
    return request;
}

function ajaxpost(url, parameters, callback, async) {
    var request = false;
    request=ajaxhttprequest('text/html');
    if (!request) {
        alert('Cannot create XMLHTTP instance');
        return false;
    }
    if (callback)
    request.onreadystatechange =  function(func) {
        return function() {
            if (request.readyState != 4)                
                return;
            callback(request);
        }(callback);
    }

    request.open('POST', url, async);
    request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    request.setRequestHeader("Content-length", parameters.length);
    request.setRequestHeader("Connection", "close");
    request.send(parameters);
    return request;
}


// Possible problem; if form is reloaded while this is running.
// TODO: make the separator work better
function ajaxpostform(url,form,callback,async) {
    var params = "";
    for(i=0; i < form.elements.length; i++)
    {
        var isformObject = false;
        var elm=form.elements[i];
        if (elm.tagName == "INPUT") {
            switch (elm.type) {
                case "text":
                case "image":
                case "hidden":
                    params += elm.name + "=" + ajaxencode(elm.value);
                    isformObject = true;
                    break;

                case "checkbox":
                    if (elm.checked) {
                        params += elm.name + "=" + ajaxencode(elm.value);
                    }else{
                        params += elm.name + "=";
                    }
                    isformObject = true;
                    break;

                case "radio":
                    if (elm.checked) {
                        params += elm.name + "=" + ajaxencode(elm.value);
                        isformObject = true;
                    }
                    break;
            }
        }

        if (elm.tagName == "SELECT") {
            params += elm.name + "=" + ajaxencode(elm.options[elm.selectedIndex].value);
            isformObject = true;
        }
        
        if (elm.tagName == "TEXTAREA") {
            params += elm.name + "=" + ajaxencode(elm.value);
            isformObject = true;
        }

        if ((isformObject) && ((i+1) < form.elements.length)) {
            params += "&";
        }
    }

    ajaxpost(url,params,callback,async);
    form.reset();
    return false; // block posting
}

function ajaxfindobj(id) {
    // portable get object
    return document.getElementById(id);
}

function ajaxfindobjs(name) {
    return document.getElementsByName(name);
}

function ajaxmonitorspawn() {
    // resceive monitor events for objects on the page
    var url=ajaxurl+'monitor/'+ajaxeventid+'/';
    debug=url;
    // alert('Monitor for '+url);
    ajaxmonitorrequest=ajaxget(url,ajaxmonitorupdate,true);
}

function ajaxmonitorabort() {
    if (ajaxmonitorrequest)
        ajaxmonitorrequest.abort();
}

function ajaxmonitor(eventid, url) {
    ajaxeventid=eventid;
    setTimeout('ajaxmonitorspawn()',1);
    setInterval('counter()',1000);
}

function ajaxobjectupdate(req) {
    if (req.status == 200) {
        var result = req.responseText;
        // must return object id in comment
        var id = result.substr(5,20).replace(/ /g,"");

        var divs= ajaxfindobjs(id);
        for (var i=0;i<divs.length; i++) {
            var obj = divs[i];
            try {
                obj.innerHTML=result;
            } catch(err) {} // ignore error
        }
        
    } else {
        try {
            div=ajaxfindobj('error');
            div.innerHTML='There was a problem updating the contents.'+debug;
        } catch(err) {} // ignore error
    }
}

function ajaxmonitorupdate(req) {
    if (req != null && req.status == 200) {
        
        // must return event in lines  
        var event=req.responseText.split('\n');
        var i;
        var id;
        for (i=0;i<event.length;i++) {
            id=event[i];
            if (id != "bye" && id != "") {
                if (id.substr(0,5)=='eval:') {
                    eval(id.substr(5,id.length)); // evaluate the expression
                } else if (id.substr(0,1)=='!') {
                    ajaxeventid=parseInt(id.substr(1,id.length));
                } else {
                    // send object update request
                    if (ajaxfindobjs(id).length) {
                        debug=ajaxurl+id;
                        ajaxget(ajaxurl+id+'/?event='+ajaxeventid,ajaxobjectupdate,true);
                    }
                }
            }
        }
        ajaxmonitorrequest=false;
        setTimeout('ajaxmonitorspawn()',1); // we get a nice return ask again right away
    } else {
        try {
            var div = ajaxfindobj('error');
            div.innerHTML='There was a problem monitoring the contents.';
        } catch(err) {} // ignore error
        ajaxmonitorrequest=false;
        setTimeout('ajaxmonitorspawn()',2000); // we get a bad return wait a bit
    }
}
