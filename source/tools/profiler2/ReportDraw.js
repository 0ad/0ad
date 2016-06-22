// Copyright (c) 2016 Wildfire Games
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Handles the drawing of a report

var g_report_draw = (function()
{
var outInterface = {};

var mouse_is_down = null;

function rebuild_canvases(raw_data)
{
    g_canvas = {};

    g_canvas.canvas_frames = $('<canvas width="1600" height="128"></canvas>').get(0);
    g_canvas.threads = {};

    for (var thread = 0; thread < raw_data.threads.length; thread++)
        g_canvas.threads[thread] = $('<canvas width="1600" height="128"></canvas>').get(0);

    g_canvas.canvas_zoom = $('<canvas width="1600" height="192"></canvas>').get(0);
    g_canvas.text_output = $('<pre></pre>').get(0);
    
    $('#timelines').empty();
    $('#timelines').append(g_canvas.canvas_frames);
    for (var thread = 0; thread < raw_data.threads.length; thread++)
        $('#timelines').append($(g_canvas.threads[thread]));

    $('#timelines').append(g_canvas.canvas_zoom);
    $('#timelines').append(g_canvas.text_output);
}
outInterface.rebuild_canvases = rebuild_canvases;

function update_display(data, range)
{
    let mainData = data.threads[g_main_thread];
    if (range.seconds)
    {
        range.tmax = mainData.frames[mainData.frames.length-1].t1;
        range.tmin = mainData.frames[mainData.frames.length-1].t1-range.seconds;
    }
    else if (range.frames)
    {
        range.tmax = mainData.frames[mainData.frames.length-1].t1;
        range.tmin = mainData.frames[mainData.frames.length-1-range.frames].t0;
    }

    $(g_canvas.text_output).empty();

    display_frames(data.threads[g_main_thread], g_canvas.canvas_frames, range);
    display_events(data.threads[g_main_thread], g_canvas.canvas_frames, range);

    set_frames_zoom_handlers(data, g_canvas.canvas_frames);
    set_tooltip_handlers(g_canvas.canvas_frames);


    $(g_canvas.canvas_zoom).unbind();

    set_zoom_handlers(data.threads[g_main_thread], data.threads[g_main_thread], g_canvas.threads[g_main_thread], g_canvas.canvas_zoom);
    set_tooltip_handlers(data.canvas_zoom);

    for (var i = 0; i < data.threads.length; i++)
    {
        $(g_canvas.threads[i]).unbind();

        let events = slice_intervals(data.threads[i], range);

        display_hierarchy(data.threads[i], events, g_canvas.threads[i], {});
        set_zoom_handlers(data.threads[i], events, g_canvas.threads[i], g_canvas.canvas_zoom);
        set_tooltip_handlers(g_canvas.threads[i]);
    };
}
outInterface.update_display = update_display;

function display_frames(data, canvas, range)
{
    canvas._tooltips = [];

    var ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.save();

    var xpadding = 8;
    var padding_top = 40;
    var width = canvas.width - xpadding*2;
    var height = canvas.height - padding_top - 4;

    var tmin = data.tmin;
    var tmax = data.tmax;
    var dx = width / (tmax-tmin);
    
    canvas._zoomData = {
        'x_to_t': x => tmin + (x - xpadding) / dx,
        't_to_x': t => (t - tmin) * dx + xpadding
    };
    
    // log 100 scale, skip < 15 ms (60fps) 
    var scale = x => 1 - Math.max(0, Math.log(1 + (x-15)/10) / Math.log(100));

    ctx.strokeStyle = 'rgb(0, 0, 0)';
    ctx.fillStyle = 'rgb(255, 255, 255)';
    for (var i = 0; i < data.frames.length; ++i)
    {
        var frame = data.frames[i];
        
        var duration = frame.t1 - frame.t0;
        var x0 = xpadding + dx*(frame.t0 - tmin);
        var x1 = x0 + dx*duration;
        var y1 = canvas.height;
        var y0 = y1 * scale(duration*1000);
        
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
                    frame.attrs.forEach(function(attr)
                    {
                        t += attr + '<br>';
                    });
                }
                return t;
            }} (frame, duration)
        });
    }

    [16, 33, 200, 500].forEach(function(t)
    {
        var y1 = canvas.height;
        var y0 = y1 * scale(t);
        var y = Math.floor(y0) + 0.5;

        ctx.beginPath();
        ctx.moveTo(xpadding, y);
        ctx.lineTo(canvas.width - xpadding, y);
        ctx.strokeStyle = 'rgb(255, 0, 0)';
        ctx.stroke();
        ctx.fillStyle = 'rgb(255, 0, 0)';
        ctx.fillText(t+'ms', 0, y-2);
    });

    ctx.strokeStyle = 'rgba(0, 0, 255, 0.5)';
    ctx.fillStyle = 'rgba(128, 128, 255, 0.2)';
    ctx.beginPath();
    ctx.rect(xpadding + dx*(range.tmin - tmin), 0, dx*(range.tmax - range.tmin), canvas.height);
    ctx.fill();
    ctx.stroke();
    
    ctx.restore();
}
outInterface.display_frames = display_frames;

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

        if (x < 2)
            continue;
        
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
outInterface.display_events = display_events;

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
    if (precision > 10)
        precision = 10;
    if (precision < 0)
        precision = 0;
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

        var x0 = Math.floor(time_to_x(interval.t0));
        var x1 = Math.floor(time_to_x(interval.t1));
        
        if (x1-x0 < 1)
            continue;
        
        var y0 = padding_top + interval.depth * BAR_SPACING;
        var y1 = y0 + BAR_SPACING;
        
        var label = interval.id;
        if (interval.attrs)
        {
            if (/^\d+$/.exec(interval.attrs[0]))
                label += ' ' + interval.attrs[0];
            else
                label += ' [...]';
        }

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
outInterface.display_hierarchy = display_hierarchy;

function set_frames_zoom_handlers(data, canvas0)
{
    function do_zoom(data, event)
    {
        var zdata = canvas0._zoomData;

        var relativeX = event.pageX - this.offsetLeft;
        var relativeY = event.pageY - this.offsetTop;
        
        var width = relativeY / canvas0.height;
        width = width*width;
        width *= zdata.x_to_t(canvas0.width)/10;

        var tavg = zdata.x_to_t(relativeX);
        var tmax = tavg + width/2;
        var tmin = tavg - width/2;
        var range = {'tmin': tmin, 'tmax': tmax};
        update_display(data, range);
    }

    $(canvas0).unbind();
    $(canvas0).mousedown(function(event)
    {
        mouse_is_down = canvas0;
        do_zoom.call(this, data, event);
    });
    $(canvas0).mouseup(function(event)
    {
        mouse_is_down = null;
    });
    $(canvas0).mousemove(function(event)
    {
        if (mouse_is_down)
            do_zoom.call(this, data, event);
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
        var relativeY = (event.pageY + this.offsetTop) / canvas0.height;
        relativeY = relativeY - 0.5;
        relativeY *= 5;
        relativeY *= relativeY;
        var width = relativeY / canvas0.height;
        width = width*width;
        width = 3 + width * x_to_time(canvas0.width)/10;
        var zoom = { tmin: x_to_time(relativeX-width/2), tmax: x_to_time(relativeX+width/2) };
        display_hierarchy(main_data, data, canvas0, hdata.range, zoom);
        display_hierarchy(main_data, data, canvas1, zoom, undefined);
        set_tooltip_handlers(canvas1);
    }

    $(canvas0).mousedown(function(event)
    {
        mouse_is_down = canvas0;
        do_zoom.call(this, event);
    });
    $(canvas0).mouseup(function(event)
    {
        mouse_is_down = null;
    });
    $(canvas0).mousemove(function(event)
    {
        if (mouse_is_down)
            do_zoom.call(this, event);
    });
}

return outInterface;
})();