from userreport.models import UserReport

from django.http import HttpResponse, HttpResponseForbidden
from django.shortcuts import render_to_response
from django.template import RequestContext
from django.core.paginator import Paginator, InvalidPage, EmptyPage
from django.db.models import Q

import re
import numpy

def render_reports(request, reports, template, args):
    paginator = Paginator(reports, args.get('pagesize', 100))
    try:
        page = int(request.GET.get('page', '1'))
    except ValueError:
        page = 1
    try:
        report_page = paginator.page(page)
    except (EmptyPage, InvalidPage):
        report_page = paginator.page(paginator.num_pages)

    args['report_page'] = report_page
    return render_to_response(template, args)


def report_user(request, user):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(user_id_hash = user)

    return render_reports(request, reports, 'reports/user.html', {'user': user})

def report_messages(request):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(data_type = 'message', data_version__gte = 1)

    return render_reports(request, reports, 'reports/message.html', {})

def report_profile(request):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(data_type = 'profile', data_version__gte = 2)

    return render_reports(request, reports, 'reports/profile.html', {'pagesize': 20})

def report_hwdetect(request):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 1)

    return render_reports(request, reports, 'reports/hwdetect.html', {})

def report_gfx(request):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 1)

    return render_reports(request, reports, 'reports/gfx.html', {'pagesize': 1000})

def report_performance(request):
    reports = UserReport.objects.order_by('upload_date')
    reports = reports.filter(
        Q(data_type = 'hwdetect', data_version__gte = 5) |
        Q(data_type = 'profile', data_version__gte = 1)
    )

    def summarise_hwdetect(report):
        json = report.data_json()
        return {
            'cpu_identifier': json['cpu_identifier'],
            'device': report.gl_device_identifier(),
            'build_debug': json['build_debug'],
            'build_revision': json['build_revision'],
            'build_datetime': json['build_datetime'],
            'gfx_res': (json['video_xres'], json['video_yres']),
        }

    def summarise_profile(report):
        json = report.data_json()
        msecs = None
        for name,table in json['profiler'].items():
            m = re.match(r'Profiling Information for: root \(Time in node: (\d+\.\d+) msec/frame\)', name)
            if m:
                #msecs = m.group(1)
                try:
                    if report.data_version == 1:
                        msecs = float(table['render']['msec/frame'])
                    else:
                        msecs = float(table['data']['render'][2])
                except KeyError:
                    pass
                # TODO: get the render time?
                # TODO: get shadow, water settings
        return {
            'msecs': msecs,
            'map': json['map'],
            'time': json['time'],
#            'json': json
        }

    profiles = []
    last_hwdetect = {}
    for report in reports:
        if report.data_type == 'hwdetect':
            last_hwdetect[report.user_id_hash] = summarise_hwdetect(report.downcast())
        elif report.data_type == 'profile':
            if report.user_id_hash in last_hwdetect:
                profiles.append([report.user_id_hash, last_hwdetect[report.user_id_hash], summarise_profile(report)])

    datapoints = {}
    for user,hwdetect,profile in profiles:
        if hwdetect['build_debug']:
            continue
#        if profile['time'] != 60:
#            continue
        if profile['msecs'] is None or float(profile['msecs']) == 0:
            continue
        datapoints.setdefault(hwdetect['device'], []).append(profile['msecs'])

#    return render_to_response('reports/performance.html', {'data': datapoints, 'reports': profiles})

    sorted_datapoints = sorted(datapoints.items(), key = lambda (k,v):
        -numpy.median(v)
    )

    data_boxplot = [ v for k,v in sorted_datapoints ]
    data_scatter = ([], [])
    for i in range(len(data_boxplot)):
        for x in data_boxplot[i]:
            data_scatter[0].append(i+1)
            data_scatter[1].append(x)

    from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
    from matplotlib.figure import Figure

    fig = Figure(figsize=(16,16))

    ax = fig.add_subplot(111)
    fig.subplots_adjust(left = 0.05, right = 0.99, top = 0.99, bottom = 0.2)

    ax.boxplot(data_boxplot, sym = '')
    ax.scatter(data_scatter[0], data_scatter[1], marker='x')

    ax.set_ylim(1, 10000)
    ax.set_yscale('log')
    ax.set_ylabel('Render time (msecs)')

    ax.set_xticklabels([k for k,v in sorted_datapoints], rotation=90, fontsize=8)

    canvas = FigureCanvas(fig)
    response = HttpResponse(content_type = 'image/png')
    canvas.print_png(response, dpi=80)
    return response
