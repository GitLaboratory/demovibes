// ****************************************************
// Insert GPL header here.
// ****************************************************

// ****************************************************
// Compatability matrix (as far as I can guess)
//
// IE5+, Mozilla 1.0+, and Netscape 6+
//

function requestsong(no) {
    $.get(ajaxurl+'song/'+no+'/queue/?'+ajaxeventid);
}

function format_time(s) {
            var r = "NaN";
            var h=Math.round(Math.floor(s/(60.0*60.0)));
            s%=(60*60);
            var m=Math.round(Math.floor(s/60.0));
            s%=(60);
            if (s<10) {
                s="0"+s;
            }
            if (h>0 && m<10) {
                m="0"+m;
            }
            try {
                if (h>0) {
                   r = h+":"+m+":"+s;
                }else{
                   r = m+":"+s;
                }
            }catch(err){} // ignore error
            return r;
        }

// added support for multiple counter spans with arbitrary direction
function counter() {
    $("[data-name='counter']").each( function (n) {
        i = $(this);
        var counter=1*i.attr("data-sec");
        var inc=1*i.attr("data-inc");
        if ((inc<0 && counter>0) || inc>0) {
            counter=counter+inc;

            i.attr("data-sec",counter);
            var s=counter;
            var h=Math.round(Math.floor(s/(60.0*60.0)));
            s%=(60*60);
            var m=Math.round(Math.floor(s/60.0));
            s%=(60);
            if (s<10) {
                s="0"+s;
            }
            if (h>0 && m<10) {
                m="0"+m;
            }
            try {
                if (h>0) {
                    i.text(h+":"+m+":"+s);
                }else{
                    i.text(m+":"+s);
                }
            }catch(err){} // ignore error
        }
    });
}

// Added try/catch to prevent errors, if updates occur while this runs.
function countdown() {
    var txt='0:00';
    if (counter>=0) {
        counter=counter-1;
    }
    if (counter>=0) {
        var s=counter;
        var h=Math.round(Math.floor(s/(60.0*60.0)));
        s%=(60*60);
        var m=Math.round(Math.floor(s/60.0));
        s%=(60);
        if (s<10) {
        s="0"+s;
        }
        if (h>0) {
            txt=h+":"+m+":"+s;
        }else{
            txt=m+":"+s;
        }
    } else {
        iscounting=0;
        clearInterval(Timer);
    }
    var divs=ajaxfindobjs('counter');
    for (var i=0;i<divs.length;i++) {
        var obj=divs[i];
        try {
            obj.innerHTML=txt;
        } catch(err) {} // ignore errors
    }
}

// Added try/catch to prevent errors, if updates occur while this runs.
function voteshow(id,value)
{
    for (i=1;i<=5;i++)
    {
        var objs=$("#"+id+'-'+i);
        for (var j=0;j<objs.length;j++)
        {
            var obj=objs[j];
            if (obj)
            {
                if (i>value)
                {
                    diff=(i-value);
                    if (diff>=1)
                    {
                        try
                        {
                            // Spaces AFTER the selected star (if on 3, 4 and 5 use this)
                            obj.src='/static/star-white.png';
                            obj.style.width='100%';
                        } catch(err) {} // ignore errors
                    } else
                    {
                        try
                        {
                            // This only gets called if value has decimals!
                            obj.src='/static/star-gold.png';
                            obj.style.width=100-(diff*100)+'%';
                        } catch(err) {} // ignore errors
                    }
                } else
                {
                    try
                    {
                        // Voting up to and under mouse pointer, or default value if not on img
                        obj.src='/static/star-red.png';
                        obj.style.width='100%';
                    } catch(err) {} // ignore errors
                }
            }
        }
    }
}

function updateOnelinerElement(linkelement, data) {
    var title = data.data.title;
    if (title.length > 21) {
        title = title.substr(0, 19) + "..";
    }
    $(linkelement).find(".yttitle").text(title);
    $(linkelement).find("img.ajaxload").remove();
    var desc = "<img class='popupscreen' src='"+data.data.thumbnail.sqDefault+"' style='float: right; padding:5px;'/>";
    desc = desc + data.data.title + " by " + data.data.uploader;
    desc = desc + "<div class='ytpopupdata'>Rating : "+ data.data.rating.toPrecision(2)+"<br/> Duration : " + format_time(data.data.duration);
    $(linkelement).qtip({
        "content": {
            "text": desc
        },
        "position": {
            "my" : "top left",
            "at" : "bottom right"
        },
           "style": {
               "classes": 'ui-tooltip-shadow ui-tooltip-jtools'
            }
    });
}

function updateElement(linkelement, data) {
    var element = $("<div/>");
    var ytid = data.data.id;
    element.addClass("youtube-link");
    $(linkelement).after(element);
    var title = data.data.title;
    var link = $(linkelement).attr("href");
    var content = "<div class='ytvidlink ytv"+ytid+"'><a href='"+link+"' target='_blank'><img src='"+data.data.thumbnail.sqDefault+"'><span class='overlay'><img src='/static/icon_youtube.png' /></span></a></div>";
    element.empty();
    element.append(content);
    $(linkelement).remove();
    var desc = data.data.description.replace(/\n/, "<br />");
    if (!desc) { desc = "<i>No description</i>" }
    desc = "<div class='ytpopupdesc'>" + desc + "</div>";
    desc = desc + "<div class='ytpopupdata'>Rating : "+ data.data.rating.toPrecision(2)+" | Views : "+ data.data.viewCount +" | Duration : " + format_time(data.data.duration);
    $(".ytv"+ytid).qtip({
        "content": {
            "text": desc,
            "title": {
                "text": title + " by " + data.data.uploader
            }
        },
        "position": {
            "my" : "top center",
            "at" : "bottom center"
        },
           "style": {
               "classes": 'ui-tooltip-shadow ui-tooltip-jtools'
            }
    })
}

var cachekey = "ytcacheY-";
var datagrab="https://gdata.youtube.com/feeds/api/videos/!YTID!?v=2&alt=jsonc&callback=?";

function get_yt_info(video_id, callback, errorcallback) {
    var content = localStorage.getItem(cachekey + video_id);
    if (!content) {
        $.getJSON(datagrab.replace("!YTID!", video_id), function(data) {
            if ((data) && (data.data)) {
                localStorage.setItem(cachekey + video_id, JSON.stringify(data));
                callback(data);
            } else {
                if (errorcallback) {
                    errorcallback();
                }
            }
        });
    } else {
        var data = JSON.parse(content);
        callback(data);
    }
}

var ytvidid_regex = /([\w_-]+)/i

function updateOnelinerLinks() {
    $(".ytlinkol").each( function (i, element) {
        var ytid = ytvidid_regex.exec($(element).data("ytid"))[0];
        $(element).append("<img src='/static/ajax-loader.gif' class='ajaxload'/>");
        get_yt_info(ytid, function(data) {
            updateOnelinerElement(element, data);
        }, function() {
            $(element).find("img.ajaxload").remove();
        });
    });
}

$(document).ready( function() {
    updateOnelinerLinks();
    $(".ytlink").each( function(i, element) {
        var ytid = $(element).data("ytid");
        $(element).append("<img src='/static/ajax-loader.gif' />");

        get_yt_info(ytid, function(data) {
            updateElement(element, data);
        }, function() {
            $(element).find("img").remove();
        });
    });
});


