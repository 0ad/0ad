from django.conf.urls.defaults import *

from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    (r'^$', 'userreport.views.index'),
    (r'^report/', include('userreport.urls')),
    (r'^private/', include('userreport.urls_private')),
    (r'^admin/', include(admin.site.urls)),
)
