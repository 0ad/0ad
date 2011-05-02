from django.conf.urls.defaults import *

urlpatterns = patterns('',
    (r'^upload/v1/$', 'userreport.views.upload'),

    (r'^opengl/$', 'userreport.views.report_opengl_index'),
    (r'^opengl/json$', 'userreport.views.report_opengl_json'),
    (r'^opengl/json/format$', 'userreport.views.report_opengl_json_format'),
    (r'^opengl/feature/(?P<feature>[^/]+)$', 'userreport.views.report_opengl_feature'),
    (r'^opengl/device/(?P<device>.+)$', 'userreport.views.report_opengl_device'),
    (r'^opengl/device', 'userreport.views.report_opengl_device_compare'),

    (r'^cpu/$', 'userreport.views.report_cpu'),

    (r'^usercount/$', 'userreport.views.report_usercount'),
)
