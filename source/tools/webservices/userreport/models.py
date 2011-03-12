from django.db import models
from django.utils import simplejson

import re

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

    def downcast(self):
        if self.data_type == 'hwdetect':
            return UserReport_hwdetect.objects.get(id = self.id)
        else:
            return self

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

    def gl_renderer(self):
        json = self.data_json()
        if json is None or 'GL_RENDERER' not in json:
            return None
        # The renderer string should typically be interpreted as UTF-8
        try:
            return json['GL_RENDERER'].encode('iso-8859-1').decode('utf-8')
        except UnicodeError:
            return json['GL_RENDERER']

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
            if k == 'GL_VERSION':
                m = re.match(r'^(\d+\.\d+).*', v)
                if m:
                    limits[k] = '%s [...]' % m.group(1)
                continue
            if k in ('GL_RENDERER', 'GL_EXTENSIONS'):
                continue
            # Hide some values that got deleted from the report in r8953, for consistency
            if k in ('GL_MAX_COLOR_MATRIX_STACK_DEPTH', 'GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB', 'GL_FRAGMENT_PROGRAM_ARB.GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB'):
                continue
            limits[k] = v
        return limits

    # Construct a nice-looking concise graphics device identifier
    # (skipping boring hardware/driver details)
    def gl_device_identifier(self):
        r = self.gl_renderer()
        m = re.match(r'^(?:ATI |Mesa DRI )?(.*?)\s*(?:GEM 20100330 DEVELOPMENT|GEM 20091221 2009Q4|20090101|Series)?\s*(?:x86|/AGP|/PCI|/MMX|/MMX\+|/SSE|/SSE2|/3DNOW!|/3DNow!\+)*(?: TCL| NO-TCL)?(?: DRI2)?$', r)
        if m:
            r = m.group(1)
        return r

    # Construct a nice string identifying the driver
    def gl_driver(self):
        json = self.data_json()
        gfx_drv_ver = json['gfx_drv_ver']

        # Try the Mesa style first
        m = re.match(r'^OpenGL \d+\.\d+(?:\.\d+)? (Mesa .*)$', gfx_drv_ver)
        if m:
            return m.group(1)

        # Try the NVIDIA Linux style
        m = re.match(r'^OpenGL \d+\.\d+(?:\.\d+)? NVIDIA (.*)$', gfx_drv_ver)
        if m:
            return m.group(1)

        # Try the ATI Catalyst Linux style
        m = re.match(r'^OpenGL (\d+\.\d+\.\d+) Compatibility Profile Context$', gfx_drv_ver)
        if m:
            return m.group(1)

        # Try to guess the relevant Windows driver
        # (These are the ones listed in lib/sysdep/os/win/wgfx.cpp)

        if json['GL_VENDOR'] == 'NVIDIA Corporation':
            # Assume 64-bit takes precedence
            m = re.search(r'nvoglv64.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)
            m = re.search(r'nvoglv32.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)
            m = re.search(r'nvoglnt.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)

        if json['GL_VENDOR'] in ('ATI Technologies Inc.', 'Advanced Micro Devices, Inc.'):
            m = re.search(r'atioglxx.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)

        if json['GL_VENDOR'] == 'Intel':
            # Assume 64-bit takes precedence
            m = re.search(r'ig4icd64.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)
            m = re.search(r'ig4icd32.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)
            # Legacy 32-bit
            m = re.search(r'iglicd32.dll \((.*?)\)', gfx_drv_ver)
            if m: return m.group(1)

        return gfx_drv_ver
