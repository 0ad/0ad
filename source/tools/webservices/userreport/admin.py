from userreport.models import UserReport
from django.contrib import admin

class UserReportAdmin(admin.ModelAdmin):

    readonly_fields = ['uploader', 'user_id_hash', 'upload_date', 'generation_date', 'data_type', 'data_version', 'data']
    fieldsets = [
        ('User', {'fields': ['uploader', 'user_id_hash']}),
        ('Dates', {'fields': ['upload_date', 'generation_date']}),
        (None, {'fields': ['data_type', 'data_version', 'data']}),
    ]
    list_display = ('uploader', 'user_id_hash', 'data_type', 'data_version', 'upload_date', 'generation_date')
    list_filter = ['upload_date', 'generation_date', 'data_type']
    search_fields = ['=uploader', '=user_id_hash']
    date_hierarchy = 'upload_date'

admin.site.register(UserReport, UserReportAdmin)
