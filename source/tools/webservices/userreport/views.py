from userreport.models import UserReport, UserReport_hwdetect
import userreport.x86 as x86
import userreport.gl

import hashlib
import datetime
import zlib

from django.http import HttpResponseBadRequest, HttpResponse, Http404, QueryDict
from django.shortcuts import render_to_response
from django.template import RequestContext
from django.utils import simplejson

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
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 4)

    all_users = set()
    cpus = {}

    for report in reports:
        json = report.data_json()
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

        cpu['caches'] = fmt_caches(json['x86_dcaches'], json['x86_icaches'], fmt_cache)
        cpu['tlbs'] = fmt_caches(json['x86_dtlbs'], json['x86_itlbs'], fmt_tlb)

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

def get_hwdetect_reports():
    reports = UserReport_hwdetect.objects
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 3)
    return reports

def report_opengl_json(request):
    reports = get_hwdetect_reports()

    devices = {}

    for report in reports:
        json = report.data_json()
        if json is None:
            continue

        exts = report.gl_extensions()
        limits = report.gl_limits()

        devices.setdefault(hashabledict({'renderer': report.gl_renderer(), 'os': report.os()}), {}).setdefault((hashabledict(limits), exts), set()).add(report.gl_driver())

    distinct_devices = []
    for (renderer, v) in devices.items():
        for (caps, versions) in v.items():
            distinct_devices.append((renderer, sorted(versions), caps))
    distinct_devices.sort(key = lambda x: (x[0]['renderer'], x[0]['os'], x))

    data = []
    for r,vs,(limits,exts) in distinct_devices:
        data.append({'device': r, 'drivers': vs, 'limits': limits, 'extensions': sorted(exts)})
    json = simplejson.dumps(data, indent=1, sort_keys=True)
    return HttpResponse(json, content_type = 'text/plain')

def report_opengl_index(request):
    reports = get_hwdetect_reports()

    all_limits = set()
    all_exts = set()
    all_devices = {}
    ext_devices = {}

    for report in reports:
        json = report.data_json()
        if json is None:
            continue

        device = report.gl_device_identifier()
        all_devices.setdefault(device, set()).add(report.user_id_hash)

        exts = report.gl_extensions()
        all_exts |= exts
        for ext in exts:
            ext_devices.setdefault(ext, set()).add(device)

        limits = report.gl_limits()
        all_limits |= set(limits.keys())

    all_limits = sorted(all_limits)
    all_exts = sorted(all_exts)

    return render_to_response('reports/opengl_index.html', {
        'all_limits': all_limits,
        'all_exts': all_exts,
        'all_devices': all_devices,
        'ext_devices': ext_devices,
        'ext_versions': userreport.gl.glext_versions,
    })

def report_opengl_feature(request, feature):
    reports = get_hwdetect_reports()

    all_values = set()
    values = {}
    is_extension = False

    for report in reports:
        json = report.data_json()
        if json is None:
            continue

        device = report.gl_device_identifier()
        exts = report.gl_extensions()
        limits = hashabledict(report.gl_limits())

        val = None
        if feature in exts:
            val = True
            is_extension = True
        elif feature in limits:
            val = limits[feature]
        all_values.add(val)

        values.setdefault(val, {}).setdefault(hashabledict({'vendor': json['GL_VENDOR'], 'renderer': report.gl_renderer(), 'os': report.os(), 'device': device}), set()).add(report.gl_driver())

    if values.keys() == [None]:
        raise Http404

    if is_extension:
        values = {
            'true': values.get(True, {}),
            'false': values.get(None, {}),
        }

    return render_to_response('reports/opengl_feature.html', {
        'feature': feature,
        'all_values': all_values,
        'values': values,
        'is_extension': is_extension,
    })

def report_opengl_devices(request, selected):
    reports = get_hwdetect_reports()

    all_limits = set()
    all_exts = set()
    all_devices = set()
    devices = {}

    for report in reports:
        json = report.data_json()
        if json is None:
            continue

        device = report.gl_device_identifier()
        all_devices.add(device)
        if device not in selected:
            continue

        exts = report.gl_extensions()
        all_exts |= exts

        limits = report.gl_limits()
        all_limits |= set(limits.keys())

        devices.setdefault(hashabledict({'device': report.gl_device_identifier(), 'os': report.os()}), {}).setdefault((hashabledict(limits), exts), set()).add(report.gl_driver())

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
    })

def report_opengl_device(request, device):
    return report_opengl_devices(request, [device])

def report_opengl_device_compare(request):
    return report_opengl_devices(request, request.GET.getlist('d'))
