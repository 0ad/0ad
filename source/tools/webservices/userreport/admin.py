from userreport.models import UserReport, GraphicsDevice, GraphicsExtension, GraphicsLimit
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
    search_fields = ['=uploader', '=user_id_hash', 'data']
    date_hierarchy = 'upload_date'

class GraphicsDeviceAdmin(admin.ModelAdmin):
    pass

class GraphicsExtensionAdmin(admin.ModelAdmin):
    pass

class GraphicsLimitAdmin(admin.ModelAdmin):
    pass

admin.site.register(UserReport, UserReportAdmin)
admin.site.register(GraphicsDevice, GraphicsDeviceAdmin)
admin.site.register(GraphicsExtension, GraphicsExtensionAdmin)
admin.site.register(GraphicsLimit, GraphicsLimitAdmin)

