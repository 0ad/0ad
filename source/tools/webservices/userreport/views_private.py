from userreport.models import UserReport

from django.http import HttpResponseForbidden
from django.shortcuts import render_to_response
from django.template import RequestContext
from django.core.paginator import Paginator, InvalidPage, EmptyPage

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
    reports = reports.filter(data_type = 'profile', data_version__gte = 1)

    return render_reports(request, reports, 'reports/profile.html', {'pagesize': 20})

def report_hwdetect(request):
    reports = UserReport.objects.order_by('-upload_date')
    reports = reports.filter(data_type = 'hwdetect', data_version__gte = 1)

    return render_reports(request, reports, 'reports/hwdetect.html', {})
