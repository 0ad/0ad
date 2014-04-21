import os, sys, re, errno
try:
    from json import loads as parse_json, dumps as compile_json
except ImportError:
    from simplejson import loads as parse_json, dumps as compile_json
import urllib2 # This should go and instead use do_url_request everywhere

from txclib.urls import API_URLS
from txclib.log import logger
from txclib.exceptions import UnknownCommandError
from txclib.paths import posix_path, native_path, posix_sep
from txclib.web import verify_ssl


def find_dot_tx(path = os.path.curdir, previous = None):
    """Return the path where .tx folder is found.

    The 'path' should be a DIRECTORY.
    This process is functioning recursively from the current directory to each
    one of the ancestors dirs.
    """
    path = os.path.abspath(path)
    if path == previous:
        return None
    joined = os.path.join(path, ".tx")
    if os.path.isdir(joined):
        return path
    else:
        return find_dot_tx(os.path.dirname(path), path)


#################################################
# Parse file filter expressions and create regex

def regex_from_filefilter(file_filter, root_path = os.path.curdir):
    """Create proper regex from <lang> expression."""
    # Force expr to be a valid regex expr (escaped) but keep <lang> intact
    expr_re = re.escape(
        posix_path(os.path.join(root_path, native_path(file_filter)))
    )
    expr_re = expr_re.replace("\\<lang\\>", '<lang>').replace(
        '<lang>', '([^%(sep)s]+)' % { 'sep': re.escape(posix_sep)})

    return "^%s$" % expr_re


TX_URLS = {
    'resource': '(?P<hostname>https?://(\w|\.|:|-)+)/projects/p/(?P<project>(\w|-)+)/resource/(?P<resource>(\w|-)+)/?$',
    'release': '(?P<hostname>https?://(\w|\.|:|-)+)/projects/p/(?P<project>(\w|-)+)/r/(?P<release>(\w|-)+)/?$',
    'project': '(?P<hostname>https?://(\w|\.|:|-)+)/projects/p/(?P<project>(\w|-)+)/?$',
}


def parse_tx_url(url):
    """
    Try to match given url to any of the valid url patterns specified in
    TX_URLS. If not match is found, we raise exception
    """
    for type_ in TX_URLS.keys():
        pattern = TX_URLS[type_]
        m = re.match(pattern, url)
        if m:
            return type_, m.groupdict()
    raise Exception(
        "tx: Malformed url given. Please refer to our docs: http://bit.ly/txautor"
    )


def get_details(api_call, username, password, *args, **kwargs):
    """
    Get the tx project info through the API.

    This function can also be used to check the existence of a project.
    """
    import base64
    url = (API_URLS[api_call] % (kwargs)).encode('UTF-8')
    verify_ssl(url)

    req = urllib2.Request(url=url)
    base64string = base64.encodestring('%s:%s' % (username, password))[:-1]
    authheader = "Basic %s" % base64string
    req.add_header("Authorization", authheader)

    try:
        fh = urllib2.urlopen(req)
        raw = fh.read()
        fh.close()
        remote_project = parse_json(raw)
    except urllib2.HTTPError, e:
        if e.code in [401, 403, 404]:
            raise e
        else:
            # For other requests, we should print the message as well
            raise Exception("Remote server replied: %s" % e.read())
    except urllib2.URLError, e:
        error = e.args[0]
        raise Exception("Remote server replied: %s" % error[1])

    return remote_project


def valid_slug(slug):
    """
    Check if a slug contains only valid characters.

    Valid chars include [-_\w]
    """
    try:
        a, b = slug.split('.')
    except ValueError:
        return False
    else:
        if re.match("^[A-Za-z0-9_-]*$", a) and re.match("^[A-Za-z0-9_-]*$", b):
            return True
        return False


def discover_commands():
    """
    Inspect commands.py and find all available commands
    """
    import inspect
    from txclib import commands

    command_table = {}
    fns = inspect.getmembers(commands, inspect.isfunction)

    for name, fn in fns:
        if name.startswith("cmd_"):
            command_table.update({
                name.split("cmd_")[1]:fn
            })

    return command_table


def exec_command(command, *args, **kwargs):
    """
    Execute given command
    """
    commands = discover_commands()
    try:
        cmd_fn = commands[command]
    except KeyError:
        raise UnknownCommandError
    cmd_fn(*args,**kwargs)


def mkdir_p(path):
    try:
        if path:
            os.makedirs(path)
    except OSError, exc: # Python >2.5
        if exc.errno == errno.EEXIST:
            pass
        else:
            raise


def confirm(prompt='Continue?', default=True):
    """
    Prompt the user for a Yes/No answer.

    Args:
        prompt: The text displayed to the user ([Y/n] will be appended)
        default: If the default value will be yes or no
    """
    valid_yes = ['Y', 'y', 'Yes', 'yes', ]
    valid_no = ['N', 'n', 'No', 'no', ]
    if default:
        prompt = prompt + '[Y/n]'
        valid_yes.append('')
    else:
        prompt = prompt + '[y/N]'
        valid_no.append('')

    ans = raw_input(prompt)
    while (ans not in valid_yes and ans not in valid_no):
        ans = raw_input(prompt)

    return ans in valid_yes


# Stuff for command line colored output

COLORS = [
    'BLACK', 'RED', 'GREEN', 'YELLOW',
    'BLUE', 'MAGENTA', 'CYAN', 'WHITE'
]

DISABLE_COLORS = False


def color_text(text, color_name, bold=False):
    """
    This command can be used to colorify command line output. If the shell
    doesn't support this or the --disable-colors options has been set, it just
    returns the plain text.

    Usage:
        print "%s" % color_text("This text is red", "RED")
    """
    if color_name in COLORS and not DISABLE_COLORS:
        return '\033[%s;%sm%s\033[0m' % (
            int(bold), COLORS.index(color_name) + 30, text)
    else:
        return text


def files_in_project(curpath):
    """
    Iterate over the files in the project.

    Return each file under ``curpath`` with its absolute name.
    """
    for root, dirs, files in os.walk(curpath, followlinks=True):
        for f in files:
            yield os.path.abspath(os.path.join(root, f))
