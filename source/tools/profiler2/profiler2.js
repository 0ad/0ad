// Copyright (C) 2016 Wildfire Games.
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

// This file is the main handler, which deals with loading reports and showing the analysis graphs
// the latter could probably be put in a separate module

// global array of Profiler2Report objects
var g_reports = [];

var g_main_thread = 0;
var g_current_report = 0;

var g_profile_path = null;
var g_active_elements = [];
var g_loading_timeout = null;

function save_as_file()
{
    $.ajax({
        url: 'http://127.0.0.1:8000/download',
        success: function () {
        },
        error: function (jqXHR, textStatus, errorThrown) {
        }
    });
}

function get_history_data(report, thread, type)
{
    var ret = {"time_by_frame":[], "max" : 0, "log_scale" : null};

    var report_data = g_reports[report].data().threads[thread];
    var interval_data = report_data.intervals;

    let data = report_data.intervals_by_type_frame[type];
    if (!data)
        return ret;

    let max = 0;
    let avg = [];
    let current_frame = 0;
    for (let i = 0; i < data.length; i++)
    {
        ret.time_by_frame.push(0);
        for (let p = 0; p < data[i].length; p++)
            ret.time_by_frame[ret.time_by_frame.length-1] += interval_data[data[i][p]].duration;
    }

    // somehow JS sorts 0.03 lower than 3e-7 otherwise
    let sorted = ret.time_by_frame.slice(0).sort((a,b) => a-b);
    ret.max = sorted[sorted.length-1];
    avg = sorted[Math.round(avg.length/2)];

    if (ret.max > avg * 3)
        ret.log_scale = true;

    return ret;
}

function draw_frequency_graph()
{
    let canvas = document.getElementById("canvas_frequency");
    canvas._tooltips = [];

    let context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);

    let legend = document.getElementById("frequency_graph").querySelector("aside");
    legend.innerHTML = "";

    if (!g_active_elements.length)
        return;

    var series_data = {};
    var use_log_scale = null;

    var x_scale = 0;
    var y_scale = 0;
    var padding = 10;

    var item_nb = 0;

    var tooltip_helper = {};

    for (let typeI in g_active_elements)
    {
        for (let rep in g_reports)
        {
            item_nb++;
            let data = get_history_data(rep, g_main_thread, g_active_elements[typeI]);
            let name = rep + "/" + g_active_elements[typeI];
            if (document.getElementById('fulln').checked)
                series_data[name] = data.time_by_frame.sort((a,b) => a-b);
            else
                series_data[name] = data.time_by_frame.filter(a=>a).sort((a,b) => a-b);
            if (series_data[name].length > x_scale)
                x_scale = series_data[name].length;
            if (data.max > y_scale)
                y_scale = data.max;
            if (use_log_scale === null && data.log_scale)
                use_log_scale = true;
        }
    }
    if (use_log_scale)
    {
        let legend_item = document.createElement("p");
        legend_item.style.borderColor = "transparent";
        legend_item.textContent = " -- log x scale -- ";
        legend.appendChild(legend_item);
    }
    let id = 0;
    for (let type in series_data)
    {
        let colour = graph_colour(id);
        let time_by_frame = series_data[type];
        let p = 0;
        let last_val = 0;

        let nb = document.createElement("p");
        nb.style.borderColor = colour;
        nb.textContent = type + " - n=" + time_by_frame.length;
        legend.appendChild(nb);

        for (var i = 0; i < time_by_frame.length; i++)
        {
            let x0 = i/time_by_frame.length*(canvas.width-padding*2) + padding;
            if (i == 0)
                x0 = 0;
            let x1 = (i+1)/time_by_frame.length*(canvas.width-padding*2) + padding;
            if (i == time_by_frame.length-1)
                x1 = (time_by_frame.length-1)*canvas.width;

            let y = time_by_frame[i]/y_scale;
            if (use_log_scale)
                y = Math.log10(1 + time_by_frame[i]/y_scale * 9);

            context.globalCompositeOperation = "lighter";

            context.beginPath();
            context.strokeStyle = colour
            context.lineWidth = 0.5;
            context.moveTo(x0,canvas.height * (1 - last_val));
            context.lineTo(x1,canvas.height * (1 - y));
            context.stroke();

            last_val = y;
            if (!tooltip_helper[Math.floor(x0)])
                tooltip_helper[Math.floor(x0)] = [];
            tooltip_helper[Math.floor(x0)].push([y, type]);
        }
        id++;
    }

    for (let i in tooltip_helper)
    {
        let tooltips = tooltip_helper[i];
        let text = "";
        for (let j in tooltips)
            if (tooltips[j][0] != undefined && text.search(tooltips[j][1])===-1)
                text += "Series " + tooltips[j][1] + ": " + time_label((tooltips[j][0])*y_scale,1) + "<br>";
        canvas._tooltips.push({
                'x0': +i, 'x1': +i+1,
                'y0': 0, 'y1': canvas.height,
                'text': function(text) { return function() { return text; } }(text)
            });
    }
    set_tooltip_handlers(canvas);

    [0.02,0.05,0.1,0.25,0.5,0.75].forEach(function(y_val)
    {
        let y = y_val;
        if (use_log_scale)
            y = Math.log10(1 + y_val * 9);

        context.beginPath();
        context.lineWidth="1";
        context.strokeStyle = "rgba(0,0,0,0.2)";
        context.moveTo(0,canvas.height * (1- y));
        context.lineTo(canvas.width,canvas.height * (1 - y));
        context.stroke();
        context.fillStyle = "gray";
        context.font = "10px Arial";
        context.textAlign="left";
        context.fillText(time_label(y*y_scale,0), 2, canvas.height * (1 - y) - 2 );
    });
}

function draw_history_graph()
{
    let canvas = document.getElementById("canvas_history");
    canvas._tooltips = [];

    let context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);

    let legend = document.getElementById("history_graph").querySelector("aside");
    legend.innerHTML = "";

    if (!g_active_elements.length)
        return;

    var series_data = {};
    var use_log_scale = null;

    var frames_nb = Infinity;
    var x_scale = 0;
    var y_scale = 0;

    var item_nb = 0;

    var tooltip_helper = {};

    for (let typeI in g_active_elements)
    {
        for (let rep in g_reports)
        {
            if (g_reports[rep].data().threads[g_main_thread].frames.length < frames_nb)
                frames_nb = g_reports[rep].data().threads[g_main_thread].frames.length;
            item_nb++;
            let data = get_history_data(rep, g_main_thread, g_active_elements[typeI]);
            if (!document.getElementById('smooth').value)
                series_data[rep + "/" + g_active_elements[typeI]] = data.time_by_frame;
            else
                series_data[rep + "/" + g_active_elements[typeI]] = smooth_1D_array(data.time_by_frame, +document.getElementById('smooth').value);
            if (data.max > y_scale)
                y_scale = data.max;
            if (use_log_scale === null && data.log_scale)
                use_log_scale = true;
        }
    }
    if (use_log_scale)
    {
        let legend_item = document.createElement("p");
        legend_item.style.borderColor = "transparent";
        legend_item.textContent = " -- log y scale -- ";
        legend.appendChild(legend_item);
    }
    canvas.width = Math.max(frames_nb,600);
    x_scale = frames_nb / canvas.width;
    let id = 0;
    for (let type in series_data)
    {
        let colour = graph_colour(id);

        let legend_item = document.createElement("p");
        legend_item.style.borderColor = colour;
        legend_item.textContent = type;
        legend.appendChild(legend_item);

        let time_by_frame = series_data[type];
        let last_val = 0;
        for (var i = 0; i < frames_nb; i++)
        {
            let smoothed_time = time_by_frame[i];//smooth_1D(time_by_frame.slice(0), i, 3);

            let y = smoothed_time/y_scale;
            if (use_log_scale)
                y = Math.log10(1 + smoothed_time/y_scale * 9);

            if (item_nb === 1)
            {
                context.beginPath();
                context.fillStyle = colour;
                context.fillRect(i/x_scale,canvas.height,1/x_scale,-y*canvas.height);
            }
            else
            {
                if ( i == frames_nb-1)
                    continue;
                context.globalCompositeOperation = "lighten";
                context.beginPath();
                context.strokeStyle = colour
                context.lineWidth = 0.5;
                context.moveTo(i/x_scale,canvas.height * (1 - last_val));
                context.lineTo((i+1)/x_scale,canvas.height * (1 - y));
                context.stroke();
            }
            last_val = y;
            if (!tooltip_helper[Math.floor(i/x_scale)])
                tooltip_helper[Math.floor(i/x_scale)] = [];
            tooltip_helper[Math.floor(i/x_scale)].push([y, type]);
        }
        id++;
    }

    for (let i in tooltip_helper)
    {
        let tooltips = tooltip_helper[i];
        let text = "Frame " + i*x_scale + "<br>";
        for (let j in tooltips)
            if (tooltips[j][0] != undefined && text.search(tooltips[j][1])===-1)
                text += "Series " + tooltips[j][1] + ": " + time_label((tooltips[j][0])*y_scale,1) + "<br>";
        canvas._tooltips.push({
                'x0': +i, 'x1': +i+1,
                'y0': 0, 'y1': canvas.height,
                'text': function(text) { return function() { return text; } }(text)
            });
    }
    set_tooltip_handlers(canvas);

    [0.1,0.25,0.5,0.75].forEach(function(y_val)
    {
        let y = y_val;
        if (use_log_scale)
            y = Math.log10(1 + y_val * 9);

        context.beginPath();
        context.lineWidth="1";
        context.strokeStyle = "rgba(0,0,0,0.2)";
        context.moveTo(0,canvas.height * (1- y));
        context.lineTo(canvas.width,canvas.height * (1 - y));
        context.stroke();
        context.fillStyle = "gray";
        context.font = "10px Arial";
        context.textAlign="left";
        context.fillText(time_label(y*y_scale,0), 2, canvas.height * (1 - y) - 2 );
    });
}

function compare_reports()
{
    let section = document.getElementById("comparison");
    section.innerHTML = "<h3>Report Comparison</h3>";

    if (g_reports.length < 2)
    {
        section.innerHTML += "<p>Too few reports loaded</p>";
        return;
    }

    if (g_active_elements.length != 1)
    {
        section.innerHTML += "<p>Too many of too few elements selected</p>";
        return;
    }

    let frames_nb = g_reports[0].data().threads[g_main_thread].frames.length;
    for (let rep in g_reports)
        if (g_reports[rep].data().threads[g_main_thread].frames.length < frames_nb)
            frames_nb = g_reports[rep].data().threads[g_main_thread].frames.length;

    if (frames_nb != g_reports[0].data().threads[g_main_thread].frames.length)
        section.innerHTML += "<p>Only the first " + frames_nb + " frames will be considered.</p>";

    let reports_data = [];

    for (let rep in g_reports)
    {
        let raw_data = get_history_data(rep, g_main_thread, g_active_elements[0]).time_by_frame;
        reports_data.push({"time_data" : raw_data.slice(0,frames_nb), "sorted_data" : raw_data.slice(0,frames_nb).sort((a,b) => a-b)});
    }

    let table_output = "<table><tr><th>Profiler Variable</th><th>Median</th><th>Maximum</th><th>% better frames</th><th>time difference per frame</th></tr>"
    for (let rep in reports_data)
    {
        let report = reports_data[rep];
        table_output += "<tr><td>Report " + rep + (rep == 0 ? " (reference)":"") + "</td>";
        // median
        table_output += "<td>" + time_label(report.sorted_data[Math.floor(report.sorted_data.length/2)]) + "</td>"
        // max
        table_output += "<td>" + time_label(report.sorted_data[report.sorted_data.length-1]) + "</td>"
        let frames_better = 0;
        let frames_diff = 0;
        for (let f in report.time_data)
        {
            if (report.time_data[f] <= reports_data[0].time_data[f])
                frames_better++;
            frames_diff += report.time_data[f] - reports_data[0].time_data[f];
        }
        table_output += "<td>" + (frames_better/frames_nb*100).toFixed(0) + "%</td><td>" + time_label((frames_diff/frames_nb),2) + "</td>";
        table_output += "</tr>";
    }
    section.innerHTML += table_output + "</table>";
}

function recompute_choices(report, thread)
{
    var choices = document.getElementById("choices").querySelector("nav");
    choices.innerHTML = "<h3>Choices</h3>";
    var types = {};
    var data = report.data().threads[thread];

    for (let i = 0; i < data.intervals.length; i++)
        types[data.intervals[i].id] = 0;

    var sorted_keys = Object.keys(types).sort();

    for (let key in sorted_keys)
    {
        let type = sorted_keys[key];
        let p = document.createElement("p");
        p.textContent = type;
        if (g_active_elements.indexOf(p.textContent) !== -1)
            p.className = "active";
        choices.appendChild(p);
        p.onclick = function()
        {
            if (g_active_elements.indexOf(p.textContent) !== -1)
            {
                p.className = "";
                g_active_elements = g_active_elements.filter( x => x != p.textContent);
                update_analysis();
                return;
            }
            g_active_elements.push(p.textContent);
            p.className = "active";
            update_analysis();
        }
    }
    update_analysis();
}

function update_analysis()
{
    compare_reports();
    draw_history_graph();
    draw_frequency_graph();
}

function load_report_from_file(evt)
{
    var file = evt.target.files[0];
    if (!file)
        return;
    load_report(false, file);
    evt.target.value = null;
}

function load_report(trylive, file)
{
    if (g_loading_timeout != undefined)
        return;

    let reportID = g_reports.length;
    let nav = document.querySelector("header nav");
    let newRep = document.createElement("p");
    newRep.textContent = file.name;
    newRep.className = "loading";
    newRep.id = "Report" + reportID;
    newRep.dataset.id = reportID;
    nav.appendChild(newRep);

    g_reports.push(Profiler2Report(on_report_loaded, trylive, file));
    g_loading_timeout = setTimeout(function() { on_report_loaded(false); }, 5000);
}

function on_report_loaded(success)
{
    let element = document.getElementById("Report" + (g_reports.length-1));
    let report = g_reports[g_reports.length-1];
    if (!success)
    {
        element.className = "fail";
        setTimeout(function() { element.parentNode.removeChild(element); clearTimeout(g_loading_timeout); g_loading_timeout = null; }, 1000 );
        g_reports = g_reports.slice(0,-1);
        if (g_reports.length === 0)
            g_current_report = null;
        return;
    }
    clearTimeout(g_loading_timeout);
    g_loading_timeout = null;
    select_report(+element.dataset.id);
    element.onclick = function() { select_report(+element.dataset.id);};
}

function select_report(id)
{
    if (g_current_report != undefined)
        document.getElementById("Report" + g_current_report).className = "";
    document.getElementById("Report" + id).className = "active";
    g_current_report = id;

    // Load up our canvas
    g_report_draw.update_display(g_reports[id],{"seconds":5});

    recompute_choices(g_reports[id], g_main_thread);
}

window.onload = function()
{
    // Try loading the report live
    load_report(true, {"name":"live"});

    // add new reports
    document.getElementById('report_load_input').addEventListener('change', load_report_from_file, false);
}
