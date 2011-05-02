from django.conf.urls.defaults import *

urlpatterns = patterns('',
    (r'^hwdetect/$', 'userreport.views_private.report_hwdetect'),
    (r'^hwdetect_test_data/$', 'userreport.views_private.report_hwdetect_test_data'),
    (r'^messages/$', 'userreport.views_private.report_messages'),
    (r'^profile/$', 'userreport.views_private.report_profile'),
    (r'^performance/$', 'userreport.views_private.report_performance'),
    (r'^gfx/$', 'userreport.views_private.report_gfx'),
    (r'^ram/$', 'userreport.views_private.report_ram'),
    (r'^os/$', 'userreport.views_private.report_os'),
    (r'^user/([0-9a-f]+)$', 'userreport.views_private.report_user'),
)
