from userreport.models import UserReport, UserReport_hwdetect, GraphicsDevice, GraphicsExtension, GraphicsLimit
import userreport.x86 as x86
import userreport.gl

import hashlib
import datetime
import zlib
import re

from django.http import HttpResponseBadRequest, HttpResponse, Http404, QueryDict
from django.shortcuts import render_to_response
from django.template import RequestContext
from django.utils import simplejson
from django.db import connection, transaction

from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_POST

class hashabledict(dict):
    def __hash__(self):
        return hash(tuple(sorted(self.items())))

@csrf_exempt
@require_POST
def upload(request):

    try:
        decompressed = zlib.decompress(request.raw_post_data)
    except zlib.error:
        return HttpResponseBadRequest('Invalid POST data.\n', content_type = 'text/plain')

    POST = QueryDict(decompressed)

    try:
        user_id = POST['user_id']
        generation_date = datetime.datetime.utcfromtimestamp(int(POST['time']))
        data_type = POST['type']
        data_version = int(POST['version'])
        data = POST['data']
    except KeyError, e:
        return HttpResponseBadRequest('Missing required fields.\n', content_type = 'text/plain')

    uploader = request.META['REMOTE_ADDR']
    # Fix the IP address if running via proxy on localhost
    if uploader == '127.0.0.1':
        try:
            uploader = request.META['HTTP_X_FORWARDED_FOR'].split(',')[0].strip()
        except KeyError:
            pass

    user_id_hash = hashlib.sha1(user_id).hexdigest()

    report = UserReport(
        uploader = uploader,
        user_id_hash = user_id_hash,
        generation_date = generation_date,
        data_type = data_type,
        data_version = data_version,
        data = data
    )
    report.save()

    return HttpResponse('OK', content_type = 'text/plain')


def index(request):
    return render_to_response('index.html')


def report_cpu(request):
    reports = UserReport_hwdetect.objects
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 4, data_version__lte = 5)
    # TODO: add v6 support

    all_users = set()
    cpus = {}

    for report in reports:
        json = report.data_json_nocache()
        if json is None:
            continue

        cpu = {}
        for x in (
            'x86_vendor', 'x86_model', 'x86_family',
            'cpu_identifier', 'cpu_frequency',
            'cpu_numprocs', 'cpu_numpackages', 'cpu_coresperpackage', 'cpu_logicalpercore',
            'cpu_numcaches',
            'cpu_pagesize', 'cpu_largepagesize',
            'numa_numnodes', 'numa_factor', 'numa_interleaved',
        ):
            cpu[x] = json[x]

        cpu['os'] = report.os()

        def fmt_size(s):
            if s % (1024*1024) == 0:
                return "%d MB" % (s / (1024*1024))
            if s % 1024 == 0:
                return "%d kB" % (s / 1024)
            return "%d B" % s

        def fmt_assoc(w):
            if w == 255:
                return 'fully-assoc'
            else:
                return '%d-way' % w

        def fmt_cache(c, t):
            types = ('?', 'D', 'I ', 'U')
            if c['type'] == 0:
                return "(Unknown %s cache)" % t
            return "L%d %s: %s (%s, shared %dx%s)" % (
                c['level'], types[c['type']], fmt_size(c['totalsize']),
                fmt_assoc(c['associativity']), c['sharedby'],
                    ('' if c['linesize'] == 64 else ', %dB line' % c['linesize'])
            )

        def fmt_tlb(c, t):
            types = ('?', 'D', 'I ', 'U')
            if c['type'] == 0:
                return "(Unknown %s TLB)" % t
            return "L%d %s: %d-entry (%s, %s page)" % (
                c['level'], types[c['type']], c['entries'],
                fmt_assoc(c['associativity']), fmt_size(c['pagesize'])
            )

        def fmt_caches(d, i, cb):
            dcaches = d[:]
            icaches = i[:]
            caches = []
            while len(dcaches) or len(icaches):
                if len(dcaches) and len(icaches) and dcaches[0] == icaches[0] and dcaches[0]['type'] == 3:
                    caches.append(cb(dcaches[0], 'U'))
                    dcaches.pop(0)
                    icaches.pop(0)
                else:
                    if len(dcaches):
                        caches.append(cb(dcaches[0], 'D'))
                        dcaches.pop(0)
                    if len(icaches):
                        caches.append(cb(icaches[0], 'I'))
                        icaches.pop(0)
            return tuple(caches)

        try:
            cpu['caches'] = fmt_caches(json['x86_dcaches'], json['x86_icaches'], fmt_cache)
            cpu['tlbs'] = fmt_caches(json['x86_dtlbs'], json['x86_itlbs'], fmt_tlb)
        except TypeError:
            continue # skip on bogus cache data

        caps = set()
        for (n,_,b) in x86.cap_bits:
            if n.endswith('[2]'):
                continue
            if json['x86_caps[%d]' % (b / 32)] & (1 << (b % 32)):
                caps.add(n)
        cpu['caps'] = frozenset(caps)

        all_users.add(report.user_id_hash)
        cpus.setdefault(hashabledict(cpu), set()).add(report.user_id_hash)

    return render_to_response('reports/cpu.html', {'cpus': cpus, 'x86_cap_descs': x86.cap_descs})

def report_opengl_json(request):
    devices = {}

    reports = GraphicsDevice.objects.all()
    for report in reports:
        exts = frozenset(e.name for e in report.graphicsextension_set.all())
        limits = dict((l.name, l.value) for l in report.graphicslimit_set.all())

        device = (report.vendor, report.renderer, report.os, report.driver)
        devices.setdefault((hashabledict(limits), exts), set()).add(device)

    sorted_devices = sorted(devices.items(), key = lambda (k,deviceset): sorted(deviceset))

    data = []
    for (limits,exts),deviceset in sorted_devices:
        devices = [
            {'vendor': v, 'renderer': r, 'os': o, 'driver': d}
            for (v,r,o,d) in sorted(deviceset)
        ]
        data.append({'devices': devices, 'limits': limits, 'extensions': sorted(exts)})
    json = simplejson.dumps(data, indent=1, sort_keys=True)
    return HttpResponse(json, content_type = 'text/plain')

def report_opengl_json_format(request):
    return render_to_response('jsonformat.html')

def report_opengl_index(request):
    cursor = connection.cursor()

    cursor.execute('''
        SELECT SUM(usercount)
        FROM userreport_graphicsdevice
    ''')
    num_users = sum(c for (c,) in cursor)

    cursor.execute('''
        SELECT name, SUM(usercount)
        FROM userreport_graphicsextension e
        JOIN userreport_graphicsdevice d
            ON e.device_id = d.id
        GROUP BY name
    ''')
    exts = cursor.fetchall()
    all_exts = set(n for n,c in exts)
    ext_devices = dict((n,c) for n,c in exts)

    cursor.execute('''
        SELECT name
        FROM userreport_graphicslimit l
        JOIN userreport_graphicsdevice d
            ON l.device_id = d.id
        GROUP BY name
    ''')
    all_limits = set(n for (n,) in cursor.fetchall())

    cursor.execute('''
        SELECT device_name, SUM(usercount)
        FROM userreport_graphicsdevice
        GROUP BY device_name
    ''')
    all_devices = dict((n,c) for n,c in cursor.fetchall())

    all_limits = sorted(all_limits)
    all_exts = sorted(all_exts)

    return render_to_response('reports/opengl_index.html', {
        'all_limits': all_limits,
        'all_exts': all_exts,
        'all_devices': all_devices,
        'ext_devices': ext_devices,
        'num_users': num_users,
        'ext_versions': userreport.gl.glext_versions,
    })

def report_opengl_feature(request, feature):
    all_values = set()
    usercounts = {}
    values = {}

    cursor = connection.cursor()

    is_extension = False
    if re.search(r'[a-z]', feature):
        is_extension = True

    if is_extension:
        cursor.execute('''
            SELECT vendor, renderer, os, driver, device_name, SUM(usercount),
                (SELECT 1 FROM userreport_graphicsextension e WHERE e.name = %s AND e.device_id = d.id) AS val
            FROM userreport_graphicsdevice d
            GROUP BY vendor, renderer, os, driver, device_name, val
        ''', [feature])

        for vendor, renderer, os, driver, device_name, usercount, val in cursor:
            val = 'true' if val else 'false'
            all_values.add(val)
            usercounts[val] = usercounts.get(val, 0) + usercount
            v = values.setdefault(val, {}).setdefault(hashabledict({
                'vendor': vendor,
                'renderer': renderer,
                'os': os,
                'device': device_name
            }), {'usercount': 0, 'drivers': set()})
            v['usercount'] += usercount
            v['drivers'].add(driver)

    else:
        cursor.execute('''
            SELECT value, vendor, renderer, os, driver, device_name, usercount
            FROM userreport_graphicslimit l
            JOIN userreport_graphicsdevice d
                ON l.device_id = d.id
            WHERE name = %s
        ''', [feature])

        for val, vendor, renderer, os, driver, device_name, usercount in cursor:
            # Convert to int/float if possible, for better sorting
            try: val = int(val)
            except ValueError:
                try: val = float(val)
                except ValueError: pass

            all_values.add(val)
            usercounts[val] = usercounts.get(val, 0) + usercount
            v = values.setdefault(val, {}).setdefault(hashabledict({
                'vendor': vendor,
                'renderer': renderer,
                'os': os,
                'device': device_name
            }), {'usercount': 0, 'drivers': set()})
            v['usercount'] += usercount
            v['drivers'].add(driver)

    if values.keys() == [] or values.keys() == ['false']:
        raise Http404

    num_users = sum(usercounts.values())

    return render_to_response('reports/opengl_feature.html', {
        'feature': feature,
        'all_values': all_values,
        'values': values,
        'is_extension': is_extension,
        'usercounts': usercounts,
        'num_users': num_users,
    })

def report_opengl_devices(request, selected):
    cursor = connection.cursor()
    cursor.execute('''
        SELECT DISTINCT device_name
        FROM userreport_graphicsdevice
    ''')
    all_devices = set(n for (n,) in cursor.fetchall())

    all_limits = set()
    all_exts = set()
    devices = {}
    gl_renderers = set()

    reports = GraphicsDevice.objects.filter(device_name__in = selected)
    for report in reports:
        exts = frozenset(e.name for e in report.graphicsextension_set.all())
        all_exts |= exts

        limits = dict((l.name, l.value) for l in report.graphicslimit_set.all())
        all_limits |= set(limits.keys())

        devices.setdefault(hashabledict({'device': report.device_name, 'os': report.os}), {}).setdefault((hashabledict(limits), exts), set()).add(report.driver)

        gl_renderers.add(report.renderer)

    if len(selected) == 1 and len(devices) == 0:
        raise Http404

    all_limits = sorted(all_limits)
    all_exts = sorted(all_exts)

    distinct_devices = []
    for (renderer, v) in devices.items():
        for (caps, versions) in v.items():
            distinct_devices.append((renderer, sorted(versions), caps))
    distinct_devices.sort(key = lambda x: (x[0]['device'], x))

    return render_to_response('reports/opengl_device.html', {
        'selected': selected,
        'all_limits': all_limits,
        'all_exts': all_exts,
        'all_devices': all_devices,
        'devices': distinct_devices,
        'gl_renderers': gl_renderers,
    })

def report_opengl_device(request, device):
    return report_opengl_devices(request, [device])

def report_opengl_device_compare(request):
    return report_opengl_devices(request, request.GET.getlist('d'))



def report_usercount(request):
    reports = UserReport.objects.raw('''
        SELECT id, upload_date, user_id_hash
        FROM userreport_userreport
        ORDER BY upload_date
    ''')

    users_by_date = {}

    for report in reports:
        t = report.upload_date.date() # group by day
        users_by_date.setdefault(t, set()).add(report.user_id_hash)

    seen_users = set()
    data_scatter = ([], [], [])
    for date,users in sorted(users_by_date.items()):
        data_scatter[0].append(date)
        data_scatter[1].append(len(users))
        data_scatter[2].append(len(users - seen_users))
        seen_users |= users

    from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
    from matplotlib.figure import Figure
    from matplotlib.dates import DateFormatter
    import matplotlib.artist

    fig = Figure(figsize=(12,6))

    ax = fig.add_subplot(111)
    fig.subplots_adjust(left = 0.08, right = 0.95, top = 0.95, bottom = 0.2)

    ax.plot(data_scatter[0], data_scatter[1], marker='o')
    ax.plot(data_scatter[0], data_scatter[2], marker='o')

    ax.legend(('Total users', 'New users'), 'upper left', frameon=False)
    matplotlib.artist.setp(ax.get_legend().get_texts(), fontsize='small')

    ax.set_ylabel('Number of users per day')

    for label in ax.get_xticklabels():
        label.set_rotation(90)
        label.set_fontsize(9)

    ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d'))

    canvas = FigureCanvas(fig)
    response = HttpResponse(content_type = 'image/png')
    canvas.print_png(response, dpi=80)
    return response
