var ajaxurl="/demovibes/ajax/";
var ajaxeventid=0; // updated later
var debug;
var ajaxmonitorrequest=false;

function apf(url, form) {
    $.post(url, $(form).serialize());
    return false;
}

function ajaxmonitorspawn() {
    // resceive monitor events for objects on the page
    var url=ajaxurl+'monitor/'+ajaxeventid+'/';
    debug=url;
    // alert('Monitor for '+url);
    ajaxmonitorrequest=$.get(url,ajaxmonitorupdate);
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

function ajaxmonitorupdate(req) {
        // must return event in lines  
        var event=req.split('\n');
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
                    $("[name='"+id+"']").load(ajaxurl+id+'/?event='+ajaxeventid)
                }
            }
        }
        ajaxmonitorrequest=false;
        setTimeout('ajaxmonitorspawn()',1); // we get a nice return ask again right away
}