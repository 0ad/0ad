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

// Various functions used by several of the tiles.

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

function graph_colour(id)
{
    var hs = [0, 1/3, 2/3, 2/4, 3/4, 1/5, 3/5, 2/5, 4/5];
    return hslToRgb(hs[id % hs.length], 0.7, 0.5, 1);
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

function time_label(t, precision = 2)
{
    if (t < 0)
        return "-" + time_label(-t, precision); 
    if (t > 1e-3)
        return (t * 1e3).toFixed(precision) + 'ms';
    else
        return (t * 1e6).toFixed(precision) + 'us';
}

function slice_intervals(data, range)
{
    if (!data.intervals.length)
        return {"tmin":0,"tmax":0,"intervals":[]};

    var tmin = 0;
    var tmax = 0;
    if (range.seconds && data.frames.length)
    {
        tmax = data.frames[data.frames.length-1].t1;
        tmin = data.frames[data.frames.length-1].t1-range.seconds;
    }
    else if (range.frames && data.frames.length)
    {
        tmax = data.frames[data.frames.length-1].t1;
        tmin = data.frames[data.frames.length-1-range.frames].t0;
    }
    else
    {
        tmax = range.tmax;
        tmin = range.tmin;
    }
    var events = { "tmin" : tmin, "tmax" : tmax, "intervals" : [] };
    for (let itv in data.intervals)
    {
        let interval = data.intervals[itv];
        if (interval.t1 > tmin && interval.t0 < tmax)
            events.intervals.push(interval);
    }
    return events;
}

function smooth_1D(array, i, distance)
{
    let value = 0;
    let total = 0;
    for (let j = i - distance; j <= i + distance; j++)
    {
        value += array[j]*(1+distance*distance - (j-i)*(j-i) );
        total += (1+distance*distance - (j-i)*(j-i) );
    }
    return value/total;
}

function smooth_1D_array(array, distance)
{
    let copied = array.slice(0);
    for (let i =0; i < array.length; ++i)
    {
        let value = 0;
        let total = 0;
        for (let j = i - distance; j <= i + distance; j++)
        {
            value += array[j]*(1+distance*distance - (j-i)*(j-i) );
            total += (1+distance*distance - (j-i)*(j-i) );
        }
        copied[i] = value/total;
    }
    return copied;
}

function set_tooltip_handlers(canvas)
{
    function do_tooltip(event)
    {
        var tooltips = canvas._tooltips;
        if (!tooltips)
            return;
        
        var relativeX = event.pageX - this.getBoundingClientRect().left - window.scrollX;
        var relativeY = event.pageY - this.getBoundingClientRect().top - window.scrollY;

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
    $(canvas).mouseleave(function(event) {
        $('#tooltip').css('visibility', 'hidden');
    });
}