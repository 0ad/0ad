// TODO: this code needs a load of cleaning up and documenting,
// and feature additions and general improvement and unrubbishing

// Item types returned by the engine
var ITEM_EVENT = 1;
var ITEM_ENTER = 2;
var ITEM_LEAVE = 3;
var ITEM_ATTRIBUTE = 4;


function hslToRgb(h, s, l, a)
{
    var r, g, b;

    if (s == 0)
    {
        r = g = b = l;
    }
    else
    {
        function hue2rgb(p, q, t)
        {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1/6) return p + (q - p) * 6 * t;
            if (t < 1/2) return q;
            if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        }

        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;
        r = hue2rgb(p, q, h + 1/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1/3);
    }

    return 'rgba(' + Math.floor(r * 255) + ',' + Math.floor(g * 255) + ',' + Math.floor(b * 255) + ',' + a + ')';
}

function new_colour(id)
{
    var hs = [0, 1/3, 2/3, 1/4, 2/4, 3/4, 1/5, 3/5, 2/5, 4/5];
    var ss = [1, 0.5];
    var ls = [0.8, 0.6, 0.9, 0.7];
    return hslToRgb(hs[id % hs.length], ss[Math.floor(id / hs.length) % ss.length], ls[Math.floor(id / (hs.length*ss.length)) % ls.length], 1);
}

var g_used_colours = {};


var g_data;

function refresh()
{
    if (1)
        refresh_live();
    else
        refresh_jsonp('../../../binaries/system/profile2.jsonp');
}

function concat_events(data)
{
    var events = [];
    data.events.forEach(function(ev) {
        ev.pop(); // remove the dummy null markers
        Array.prototype.push.apply(events, ev);
    });
    return events;
}

function refresh_jsonp(url)
{
    var script = document.createElement('script');
    
    window.profileDataCB = function(data)
    {
        script.parentNode.removeChild(script);

        var threads = [];
        data.threads.forEach(function(thread) {
            var canvas = $('<canvas width="1600" height="160"></canvas>');
            threads.push({'name': thread.name, 'data': { 'events': concat_events(thread.data) }, 'canvas': canvas.get(0)});
        });
        g_data = { 'threads': threads };

        var range = {'seconds': 0.05};

        rebuild_canvases();
        update_display(range);
    };

    script.src = url;
    document.body.appendChild(script);
}

function refresh_live()
{
    $.ajax({
        url: 'http://127.0.0.1:8000/overview',
        dataType: 'json',
        success: function (data) {
            var threads = [];
            data.threads.forEach(function(thread) {
                var canvas = $('<canvas width="1600" height="160"></canvas>');
                threads.push({'name': thread.name, 'canvas': canvas.get(0)});
            });
            var callback_data = { 'threads': threads, 'completed': 0 };
            threads.forEach(function(thread) {
                refresh_thread(thread, callback_data);
            });
        },
        error: function (jqXHR, textStatus, errorThrown) {
            alert('Failed to connect to server ("'+textStatus+'")');
        }
    });
}

function refresh_thread(thread, callback_data)
{
    $.ajax({
        url: 'http://127.0.0.1:8000/query',
        dataType: 'json',
        data: { 'thread': thread.name },
        success: function (data) {
            data.events = concat_events(data);
            
            thread.data = data;
            
            if (++callback_data.completed == callback_data.threads.length)
            {
                g_data = { 'threads': callback_data.threads };

                //var range = {'numframes': 5};
                var range = {'seconds': 0.05};

                rebuild_canvases();
                update_display(range);
            }
        },
        error: function (jqXHR, textStatus, errorThrown) {
            alert('Failed to connect to server ("'+textStatus+'")');
        }
    });
}

function rebuild_canvases()
{
    g_data.canvas_frames = $('<canvas width="1600" height="128"></canvas>').get(0);
    g_data.canvas_zoom = $('<canvas width="1600" height="128"></canvas>').get(0);
    g_data.text_output = $('<pre></pre>').get(0);
    
    set_frames_zoom_handlers(g_data.canvas_frames);
    set_tooltip_handlers(g_data.canvas_frames);

    $('#timelines').empty();
    $('#timelines').append(g_data.canvas_frames);
    g_data.threads.forEach(function(thread) {
        $('#timelines').append($(thread.canvas));
    });
    $('#timelines').append(g_data.canvas_zoom);
    $('#timelines').append(g_data.text_output);
}

function update_display(range)
{
    $(g_data.text_output).empty();

    var main_events = g_data.threads[0].data.events;

    var processed_main = g_data.threads[0].processed_events = compute_intervals(main_events, range);

//    display_top_items(main_events, g_data.text_output);

    display_frames(processed_main, g_data.canvas_frames);
    display_events(processed_main, g_data.canvas_frames);

    $(g_data.threads[0].canvas).unbind();
    $(g_data.canvas_zoom).unbind();
    display_hierarchy(processed_main, processed_main, g_data.threads[0].canvas, {}, undefined);
    set_zoom_handlers(processed_main, processed_main, g_data.threads[0].canvas, g_data.canvas_zoom);
    set_tooltip_handlers(g_data.threads[0].canvas);
    set_tooltip_handlers(g_data.canvas_zoom);

    g_data.threads.slice(1).forEach(function(thread) {
        var processed_data = compute_intervals(thread.data.events, {'tmin': processed_main.tmin, 'tmax': processed_main.tmax});

        $(thread.canvas).unbind();
        display_hierarchy(processed_main, processed_data, thread.canvas, {}, undefined);
        set_zoom_handlers(processed_main, processed_data, thread.canvas, g_data.canvas_zoom);
        set_tooltip_handlers(thread.canvas);
    });
 }

function display_top_items(data, output)
{
    var items = {};
    for (var i = 0; i < data.length; ++i)
    {
        var type = data[i][0];
        if (!(type == ITEM_EVENT || type == ITEM_ENTER || type == ITEM_LEAVE))
            continue;
        var id = data[i][2];
        if (!items[id])
            items[id] = { 'count': 0 };
        items[id].count++;
    }

    var topitems = [];
    for (var k in items)
        topitems.push([k, items[k].count]);
    topitems.sort(function(a, b) {
        return b[1] - a[1];
    });
    topitems.splice(16);
    
    topitems.forEach(function(item) {
        output.appendChild(document.createTextNode(item[1] + 'x -- ' + item[0] + '\n'));
    });
    output.appendChild(document.createTextNode('(' + data.length + ' items)'));
}

function compute_intervals(data, range)
{
    var start, end;
    var tmin, tmax;
    
    var frames = [];
    var last_frame_time = undefined;
    for (var i = 0; i < data.length; ++i)
    {
        if (data[i][0] == ITEM_EVENT && data[i][2] == '__framestart')
        {
            var t = data[i][1];
            if (last_frame_time)
                frames.push({'t0': last_frame_time, 't1': t});
            last_frame_time = t;
        }
    }
    
    if (range.numframes)
    {
        for (var i = data.length - 1; i > 0; --i)
        {
            if (data[i][0] == ITEM_EVENT && data[i][2] == '__framestart')
            {
                end = i;
                break;
            }
        }
        
        var framesfound = 0;
        for (var i = end - 1; i > 0; --i)
        {
            if (data[i][0] == ITEM_EVENT && data[i][2] == '__framestart')
            {
                start = i;
                if (++framesfound == range.numframes)
                    break;
            }
        }
        
        tmin = data[start][1];
        tmax = data[end][1];
    }
    else if (range.seconds)
    {
        var end = data.length - 1;
        for (var i = end; i > 0; --i)
        {
            var type = data[i][0];
            if (type == ITEM_EVENT || type == ITEM_ENTER || type == ITEM_LEAVE)
            {
                tmax = data[i][1];
                break;
            }
        }
        tmin = tmax - range.seconds;
        
        for (var i = end; i > 0; --i)
        {
            var type = data[i][0];
            if ((type == ITEM_EVENT || type == ITEM_ENTER || type == ITEM_LEAVE) && data[i][1] < tmin)
                break;
            start = i;
        }
    }
    else
    {
        start = 0;
        end = data.length - 1;
        tmin = range.tmin;
        tmax = range.tmax;

        for (var i = data.length-1; i > 0; --i)
        {
            var type = data[i][0];
            if ((type == ITEM_EVENT || type == ITEM_ENTER || type == ITEM_LEAVE) && data[i][1] < tmax)
            {
                end = i;
                break;
            }
        }

        for (var i = end; i > 0; --i)
        {
            var type = data[i][0];
            if ((type == ITEM_EVENT || type == ITEM_ENTER || type == ITEM_LEAVE) && data[i][1] < tmin)
                break;
            start = i;
        }

        // Move the start/end outwards by another frame, so we don't lose data at the edges
        while (start > 0)
        {
            --start;
            if (data[start][0] == ITEM_EVENT && data[start][2] == '__framestart')
                break;
        }
        while (end < data.length-1)
        {
            ++end;
            if (data[end][0] == ITEM_EVENT && data[end][2] == '__framestart')
                break;
        }
    }

    var num_colours = 0;
    
    var events = [];

    // Read events for the entire data period (not just start..end)
    var lastWasEvent = false;
    for (var i = 0; i < data.length; ++i)
    {
        if (data[i][0] == ITEM_EVENT)
        {
            events.push({'t': data[i][1], 'id': data[i][2]});
            lastWasEvent = true;
        }
        else if (data[i][0] == ITEM_ATTRIBUTE)
        {
            if (lastWasEvent)
            {
                if (!events[events.length-1].attrs)
                    events[events.length-1].attrs = [];
                events[events.length-1].attrs.push(data[i][1]);
            }
        }
        else
        {
            lastWasEvent = false;
        }
    }
    
    
    var intervals = [];
    
    // Read intervals from the focused data period (start..end)
    var stack = [];
    var lastT = 0;
    var lastWasEvent = false;
    for (var i = start; i <= end; ++i)
    {
        if (data[i][0] == ITEM_EVENT)
        {
//            if (data[i][1] < lastT)
//                console.log('Time went backwards: ' + (data[i][1] - lastT));

            lastT = data[i][1];
            lastWasEvent = true;
        }
        else if (data[i][0] == ITEM_ENTER)
        {
//            if (data[i][1] < lastT)
//                console.log('Time went backwards: ' + (data[i][1] - lastT));

            stack.push({'t0': data[i][1], 'id': data[i][2]});

            lastT = data[i][1];
            lastWasEvent = false;
        }
        else if (data[i][0] == ITEM_LEAVE)
        {
//            if (data[i][1] < lastT)
//                console.log('Time went backwards: ' + (data[i][1] - lastT));

            lastT = data[i][1];
            lastWasEvent = false;
            
            if (!stack.length)
                continue;
            var interval = stack.pop();

            if (data[i][2] != interval.id && data[i][2] != '(ProfileStop)')
                alert('inconsistent interval ids ('+interval.id+' / '+data[i][2]+')');

            if (!g_used_colours[interval.id])
                g_used_colours[interval.id] = new_colour(num_colours++);
            
            interval.colour = g_used_colours[interval.id];
                
            interval.t1 = data[i][1];
            interval.duration = interval.t1 - interval.t0;
            interval.depth = stack.length;
            
            intervals.push(interval);
        }
        else if (data[i][0] == ITEM_ATTRIBUTE)
        {
            if (!lastWasEvent && stack.length)
            {
                if (!stack[stack.length-1].attrs)
                    stack[stack.length-1].attrs = [];
                stack[stack.length-1].attrs.push(data[i][1]);
            }
        }
    }

    return { 'frames': frames, 'events': events, 'intervals': intervals, 'tmin': tmin, 'tmax': tmax };
}

function time_label(t)
{
    if (t > 1e-3)
        return (t * 1e3).toFixed(2) + 'ms';
    else
        return (t * 1e6).toFixed(2) + 'us';
}

function display_frames(data, canvas)
{
    canvas._tooltips = [];

    var ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.save();

    var xpadding = 8;
    var padding_top = 40;
    var width = canvas.width - xpadding*2;
    var height = canvas.height - padding_top - 4;

    var tmin = data.frames[0].t0;
    var tmax = data.frames[data.frames.length-1].t1;
    var dx = width / (tmax-tmin);
    
    canvas._zoomData = {
        'x_to_t': function(x) {
            return tmin + (x - xpadding) / dx;
        },
        't_to_x': function(t) {
            return (t - tmin) * dx + xpadding;
        }
    };
    
//    var y_per_second = 1000;
    var y_per_second = 100;

    [16, 33, 200, 500].forEach(function(t) {
        var y1 = canvas.height;
        var y0 = y1 - t/1000*y_per_second;
        var y = Math.floor(y0) + 0.5;

        ctx.beginPath();
        ctx.moveTo(xpadding, y);
        ctx.lineTo(canvas.width - xpadding, y);
        ctx.strokeStyle = 'rgb(255, 0, 0)';
        ctx.stroke();
        ctx.fillStyle = 'rgb(255, 0, 0)';
        ctx.fillText(t+'ms', 0, y-2);
    });

    ctx.strokeStyle = 'rgb(0, 0, 0)';
    ctx.fillStyle = 'rgb(255, 255, 255)';
    for (var i = 0; i < data.frames.length; ++i)
    {
        var frame = data.frames[i];
        
        var duration = frame.t1 - frame.t0;
        var x0 = xpadding + dx*(frame.t0 - tmin);
        var x1 = x0 + dx*duration;
        var y1 = canvas.height;
        var y0 = y1 - duration*y_per_second;
        
        ctx.beginPath();
        ctx.rect(x0, y0, x1-x0, y1-y0);
        ctx.stroke();
        
        canvas._tooltips.push({
            'x0': x0, 'x1': x1,
            'y0': y0, 'y1': y1,
            'text': function(frame, duration) { return function() {
                var t = '<b>Frame</b><br>';
                t += 'Length: ' + time_label(duration) + '<br>';
                if (frame.attrs)
                {
                    frame.attrs.forEach(function(attr) {
                        t += attr + '<br>';
                    });
                }
                return t;
            }} (frame, duration)
        });
    }

    ctx.strokeStyle = 'rgba(0, 0, 255, 0.5)';
    ctx.fillStyle = 'rgba(128, 128, 255, 0.2)';
    ctx.beginPath();
    ctx.rect(xpadding + dx*(data.tmin - tmin), 0, dx*(data.tmax - data.tmin), canvas.height);
    ctx.fill();
    ctx.stroke();
    
    ctx.restore();
}

function display_events(data, canvas)
{
    var ctx = canvas.getContext('2d');
    ctx.save();
    
    var x_to_time = canvas._zoomData.x_to_t;
    var time_to_x = canvas._zoomData.t_to_x;
    
    for (var i = 0; i < data.events.length; ++i)
    {
        var event = data.events[i];
        
        if (event.id == '__framestart')
            continue;
        
        if (event.id == 'gui event' && event.attrs && event.attrs[0] == 'type: mousemove')
            continue;
        
        var x = time_to_x(event.t);
        var y = 32;
        
        var x0 = x;
        var x1 = x;
        var y0 = y-4;
        var y1 = y+4;

        ctx.strokeStyle = 'rgb(255, 0, 0)';
        ctx.beginPath();
        ctx.moveTo(x0, y0);
        ctx.lineTo(x1, y1);
        ctx.stroke();
        canvas._tooltips.push({
            'x0': x0, 'x1': x1,
            'y0': y0, 'y1': y1,
            'text': function(event) { return function() {
                var t = '<b>' + event.id + '</b><br>';
                if (event.attrs)
                {
                    event.attrs.forEach(function(attr) {
                        t += attr + '<br>';
                    });
                }
                return t;
            }} (event)
        });
    }
    
    ctx.restore();
}

function display_hierarchy(main_data, data, canvas, range, zoom)
{
    canvas._tooltips = [];

    var ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.save();

    ctx.font = '12px sans-serif';

    var xpadding = 8;
    var padding_top = 40;
    var width = canvas.width - xpadding*2;
    var height = canvas.height - padding_top - 4;
    
    var tmin, tmax, start, end;

    if (range.tmin)
    {
        tmin = range.tmin;
        tmax = range.tmax;
    }
    else
    {
        tmin = data.tmin;
        tmax = data.tmax;
    }

    canvas._hierarchyData = { 'range': range, 'tmin': tmin, 'tmax': tmax };
    
    function time_to_x(t)
    {
        return xpadding + (t - tmin) / (tmax - tmin) * width;
    }
    
    function x_to_time(x)
    {
        return tmin + (x - xpadding) * (tmax - tmin) / width;
    }

    ctx.save();
    ctx.textAlign = 'center';
    ctx.strokeStyle = 'rgb(192, 192, 192)';
    ctx.beginPath();
    var precision = -3;
    while ((tmax-tmin)*Math.pow(10, 3+precision) < 25)
        ++precision;
    var ticks_per_sec = Math.pow(10, 3+precision);
    var major_tick_interval = 5;
    for (var i = 0; i < (tmax-tmin)*ticks_per_sec; ++i)
    {
        var major = (i % major_tick_interval == 0);
        var x = Math.floor(time_to_x(tmin + i/ticks_per_sec));
        ctx.moveTo(x-0.5, padding_top - (major ? 4 : 2));
        ctx.lineTo(x-0.5, padding_top + height);
        if (major)
            ctx.fillText((i*1000/ticks_per_sec).toFixed(precision), x, padding_top - 8);
    }
    ctx.stroke();
    ctx.restore();

    var BAR_SPACING = 16;
    
    for (var i = 0; i < data.intervals.length; ++i)
    {
        var interval = data.intervals[i];
        
        if (interval.tmax <= tmin || interval.tmin > tmax)
            continue;

        var label = interval.id;
        if (interval.attrs)
        {
            if (/^\d+$/.exec(interval.attrs[0]))
                label += ' ' + interval.attrs[0];
            else
                label += ' [...]';
        }
        var x0 = Math.floor(time_to_x(interval.t0));
        var x1 = Math.floor(time_to_x(interval.t1));
        var y0 = padding_top + interval.depth * BAR_SPACING;
        var y1 = y0 + BAR_SPACING;
        
        ctx.fillStyle = interval.colour;
        ctx.strokeStyle = 'black';
        ctx.beginPath();
        ctx.rect(x0-0.5, y0-0.5, x1-x0, y1-y0);
        ctx.fill();
        ctx.stroke();
        ctx.fillStyle = 'black';
        ctx.fillText(label, x0+2, y0+BAR_SPACING-4, Math.max(1, x1-x0-4));
        
        canvas._tooltips.push({
            'x0': x0, 'x1': x1,
            'y0': y0, 'y1': y1,
            'text': function(interval) { return function() {
                var t = '<b>' + interval.id + '</b><br>';
                t += 'Length: ' + time_label(interval.duration) + '<br>';
                if (interval.attrs)
                {
                    interval.attrs.forEach(function(attr) {
                        t += attr + '<br>';
                    });
                }
                return t;
            }} (interval)
        });

    }

    for (var i = 0; i < main_data.frames.length; ++i)
    {
        var frame = main_data.frames[i];

        if (frame.t0 < tmin || frame.t0 > tmax)
            continue;

        var x = Math.floor(time_to_x(frame.t0));
        
        ctx.save();
        ctx.lineWidth = 3;
        ctx.strokeStyle = 'rgba(0, 0, 255, 0.5)';
        ctx.beginPath();
        ctx.moveTo(x+0.5, 0);
        ctx.lineTo(x+0.5, canvas.height);
        ctx.stroke();
        ctx.fillText(((frame.t1 - frame.t0) * 1000).toFixed(0)+'ms', x+2, padding_top - 24);
        ctx.restore();
    }

    if (zoom)
    {
        var x0 = time_to_x(zoom.tmin);
        var x1 = time_to_x(zoom.tmax);
        ctx.strokeStyle = 'rgba(0, 0, 255, 0.5)';
        ctx.fillStyle = 'rgba(128, 128, 255, 0.2)';
        ctx.beginPath();
        ctx.moveTo(x0+0.5, 0.5);
        ctx.lineTo(x1+0.5, 0.5);
        ctx.lineTo(x1+0.5 + 4, canvas.height-0.5);
        ctx.lineTo(x0+0.5 - 4, canvas.height-0.5);
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
    }
    
    ctx.restore();
}

function set_frames_zoom_handlers(canvas0)
{
    function do_zoom(event)
    {
        var zdata = canvas0._zoomData;

        var relativeX = event.pageX - this.offsetLeft;
        var relativeY = event.pageY - this.offsetTop;
        
//        var width = 0.001 + 0.5 * relativeY / canvas0.height;
        var width = 0.001 + 5 * relativeY / canvas0.height;

        var tavg = zdata.x_to_t(relativeX);
        var tmax = tavg + width/2;
        var tmin = tavg - width/2;
        var range = {'tmin': tmin, 'tmax': tmax};
        update_display(range);
    }

    var mouse_is_down = false;
    $(canvas0).unbind();
    $(canvas0).mousedown(function(event) {
        mouse_is_down = true;
        do_zoom.call(this, event);
    });
    $(canvas0).mouseup(function(event) {
        mouse_is_down = false;
    });
    $(canvas0).mousemove(function(event) {
        if (mouse_is_down)
            do_zoom.call(this, event);
    });
}
 
function set_zoom_handlers(main_data, data, canvas0, canvas1)
{
    function do_zoom(event)
    {
        var hdata = canvas0._hierarchyData;
        
        function x_to_time(x)
        {
            return hdata.tmin + x * (hdata.tmax - hdata.tmin) / canvas0.width;
        }
        
        var relativeX = event.pageX - this.offsetLeft;
        var relativeY = event.pageY - this.offsetTop;
        var width = 8 + 64 * relativeY / canvas0.height;
        var zoom = { tmin: x_to_time(relativeX-width/2), tmax: x_to_time(relativeX+width/2) };
        display_hierarchy(main_data, data, canvas0, hdata.range, zoom);
        display_hierarchy(main_data, data, canvas1, zoom, undefined);
    }

    var mouse_is_down = false;
    $(canvas0).mousedown(function(event) {
        mouse_is_down = true;
        do_zoom.call(this, event);
    });
    $(canvas0).mouseup(function(event) {
        mouse_is_down = false;
    });
    $(canvas0).mousemove(function(event) {
        if (mouse_is_down)
            do_zoom.call(this, event);
    });
}

function set_tooltip_handlers(canvas)
{
    function do_tooltip(event)
    {
        var tooltips = canvas._tooltips;
        if (!tooltips)
            return;
        
        var relativeX = event.pageX - this.offsetLeft;
        var relativeY = event.pageY - this.offsetTop;

        var text = undefined;
        for (var i = 0; i < tooltips.length; ++i)
        {
            var t = tooltips[i];
            if (t.x0-1 <= relativeX && relativeX <= t.x1+1 && t.y0 <= relativeY && relativeY <= t.y1)
            {
                text = t.text();
                break;
            }
        }
        if (text)
        {
            if (text.length > 512)
                $('#tooltip').addClass('long');
            else
                $('#tooltip').removeClass('long');
            $('#tooltip').css('left', (event.pageX+16)+'px');
            $('#tooltip').css('top', (event.pageY+8)+'px');
            $('#tooltip').html(text);
            $('#tooltip').css('visibility', 'visible');
        }
        else
        {
            $('#tooltip').css('visibility', 'hidden');
        }
    }

    $(canvas).mousemove(function(event) {
        do_tooltip.call(this, event);
    });
}

function search_regions(query)
{
    var re = new RegExp(query);
    var data = g_data.threads[0].processed_events;
    
    var found = [];
    for (var i = 0; i < data.intervals.length; ++i)
    {
        var interval = data.intervals[i];
        if (interval.id.match(re))
        {
            found.push(interval);
            if (found.length > 100)
                break;
        }
    }
    
    var out = $('#regionsearchresult > tbody');
    out.empty();
    for (var i = 0; i < found.length; ++i)
    {
        out.append($('<tr><td>?</td><td>' + found[i].id + '</td><td>' + (found[i].duration*1000) + '</td></tr>'));
    }
}