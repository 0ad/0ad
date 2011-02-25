from django.db import models
from django.utils import simplejson

class UserReport(models.Model):

    uploader = models.IPAddressField(editable = False)

    # Hex SHA-1 digest of user's reported ID
    # (The hashing means that publishing the database won't let people upload
    # faked reports under someone else's user ID, and also ensures a simple
    # consistent structure)
    user_id_hash = models.CharField(max_length = 40, db_index = True, editable = False)

    # When the server received the upload
    upload_date = models.DateTimeField(auto_now_add = True, db_index = True, editable = False)

    # When the user claims to have generated the report
    generation_date = models.DateTimeField(editable = False)

    data_type = models.CharField(max_length = 16, db_index = True, editable = False)

    data_version = models.IntegerField(editable = False)

    data = models.TextField(editable = False)

    def data_json(self):
        if not hasattr(self, 'cached_json'):
            try:
                self.cached_json = simplejson.loads(self.data)
            except:
                self.cached_json = None
        return self.cached_json

class UserReport_hwdetect(UserReport):

    class Meta:
        proxy = True

    def os(self):
        os = 'Unknown'
        json = self.data_json()
        if json:
            if json['os_win']:
                os = 'Windows'
            elif json['os_linux']:
                os = 'Linux'
            elif json['os_macosx']:
                os = 'OS X'
        return os

    def gl_extensions(self):
        json = self.data_json()
        if json is None or 'GL_EXTENSIONS' not in json:
            return None
        return frozenset(json['GL_EXTENSIONS'].strip().split(' '))

    def gl_limits(self):
        json = self.data_json()
        if json is None:
            return None

        limits = {}
        for (k, v) in json.items():
            if not k.startswith('GL_'):
                continue
            if k in ('GL_RENDERER', 'GL_VENDOR', 'GL_EXTENSIONS'):
                continue
            limits[k] = v
        return limits
