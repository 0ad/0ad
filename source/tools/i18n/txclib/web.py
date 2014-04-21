# -*- coding: utf-8 -*-

import os
import urllib2
import socket
import ssl
import urlparse
import mimetools
import mimetypes
import platform
#from pkg_resources import resource_filename, resource_string
from txclib import get_version
from txclib.packages.ssl_match_hostname import match_hostname


# Helper class to enable urllib2 to handle PUT/DELETE requests as well
class RequestWithMethod(urllib2.Request):
    """Workaround for using DELETE with urllib2"""

    def __init__(self, url, method, data=None, headers={},
        origin_req_host=None, unverifiable=False):
        self._method = method
        urllib2.Request.__init__(self, url, data=data, headers=headers,
                 origin_req_host=None, unverifiable=False)

    def get_method(self):
        return self._method


import urllib
import stat
from cStringIO import StringIO


class Callable:
    def __init__(self, anycallable):
        self.__call__ = anycallable

# Controls how sequences are uncoded. If true, elements may be given multiple
# values by assigning a sequence.
doseq = 1


class MultipartPostHandler(urllib2.BaseHandler):
    handler_order = urllib2.HTTPHandler.handler_order - 10 # needs to run first

    def http_request(self, request):
        data = request.get_data()
        if data is not None and type(data) != str:
            v_files = []
            v_vars = []
            try:
                 for (key, value) in data.items():
                     if type(value) == file:
                         v_files.append((key, value))
                     else:
                         v_vars.append((key, value))
            except TypeError:
                systype, value, traceback = sys.exc_info()
                raise TypeError, "not a valid non-string sequence or mapping object", traceback

            if len(v_files) == 0:
                data = urllib.urlencode(v_vars, doseq)
            else:
                boundary, data = self.multipart_encode(v_vars, v_files)

                contenttype = 'multipart/form-data; boundary=%s' % boundary
                if(request.has_header('Content-Type')
                   and request.get_header('Content-Type').find('multipart/form-data') != 0):
                    print "Replacing %s with %s" % (request.get_header('content-type'), 'multipart/form-data')
                request.add_unredirected_header('Content-Type', contenttype)

            request.add_data(data)

        return request

    def multipart_encode(vars, files, boundary = None, buf = None):
        if boundary is None:
            boundary = mimetools.choose_boundary()
        if buf is None:
            buf = StringIO()
        for(key, value) in vars:
            buf.write('--%s\r\n' % boundary)
            buf.write('Content-Disposition: form-data; name="%s"' % key)
            buf.write('\r\n\r\n' + value + '\r\n')
        for(key, fd) in files:
            file_size = os.fstat(fd.fileno())[stat.ST_SIZE]
            filename = fd.name.split(os.path.sep)[-1]
            contenttype = mimetypes.guess_type(filename)[0] or 'application/octet-stream'
            buf.write('--%s\r\n' % boundary)
            buf.write('Content-Disposition: form-data; name="%s"; filename="%s"\r\n' % (key, filename))
            buf.write('Content-Type: %s\r\n' % contenttype)
            # buffer += 'Content-Length: %s\r\n' % file_size
            fd.seek(0)
            buf.write('\r\n' + fd.read() + '\r\n')
        buf.write('--' + boundary + '--\r\n\r\n')
        buf = buf.getvalue()
        return boundary, buf
    multipart_encode = Callable(multipart_encode)

    https_request = http_request


def user_agent_identifier():
    """Return the user agent for the client."""
    client_info = (get_version(), platform.system(), platform.machine())
    return "txclient/%s (%s %s)" % client_info


def _verify_ssl(hostname, port=443):
    """Verify the SSL certificate of the given host."""
    sock = socket.create_connection((hostname, port))
    try:
        ssl_sock = ssl.wrap_socket(
            sock, cert_reqs=ssl.CERT_REQUIRED, ca_certs=certs_file()
        )
        match_hostname(ssl_sock.getpeercert(), hostname)
    finally:
        sock.close()


def certs_file():
    if platform.system() == 'Windows':
        # Workaround py2exe and resource_filename incompatibility.
        # Store the content in the filesystem permanently.
        app_dir = os.path.join(
            os.getenv('appdata', os.path.expanduser('~')), 'transifex-client'
        )
        if not os.path.exists(app_dir):
            os.mkdir(app_dir)
        ca_file = os.path.join(app_dir, 'cacert.pem')
        if not os.path.exists(ca_file):
            content = resource_string(__name__, 'cacert.pem')
            with open(ca_file, 'w') as f:
                f.write(content)
        return ca_file
    else:
        POSSIBLE_CA_BUNDLE_PATHS = [
            # Red Hat, CentOS, Fedora and friends
            # (provided by the ca-certificates package):
            '/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem',
            '/etc/ssl/certs/ca-bundle.crt',
            '/etc/pki/tls/certs/ca-bundle.crt',
            # Ubuntu, Debian, and friends
            # (provided by the ca-certificates package):
            '/etc/ssl/certs/ca-certificates.crt',
            # FreeBSD (provided by the ca_root_nss package):
            '/usr/local/share/certs/ca-root-nss.crt',
            # openSUSE (provided by the ca-certificates package),
            # the 'certs' directory is the
            # preferred way but may not be supported by the SSL module,
            # thus it has 'ca-bundle.pem'
            # as a fallback (which is generated from pem files in the
            # 'certs' directory):
            '/etc/ssl/ca-bundle.pem',
        ]
        for path in POSSIBLE_CA_BUNDLE_PATHS:
            if os.path.exists(path):
                return path
        return resource_filename(__name__, 'cacert.pem')


def verify_ssl(host):
    parts = urlparse.urlparse(host)
    if parts.scheme != 'https':
        return

    if ':' in parts.netloc:
        hostname, port = parts.netloc.split(':')
    else:
        hostname = parts.netloc
        if parts.port is not None:
            port = parts.port
        else:
            port = 443
    _verify_ssl(hostname, port)
