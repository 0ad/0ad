# coding=utf-8

from django import template
from django.template.defaultfilters import stringfilter
from django.utils import simplejson
from django.utils.safestring import mark_safe
from django.utils.html import conditional_escape

register = template.Library()

@register.filter
def mod(value, arg):
    return value % arg

@register.filter
@stringfilter
def has_token(value, token):
    "Returns whether a space-separated list of tokens contains a given token"
    return token in value.split(' ')

@register.filter
@stringfilter
def wrap_at_underscores(value):
    return value.replace('_', '&#x200b;_')
wrap_at_underscores.is_safe = True

@register.filter
@stringfilter
def prettify_json(value):
    try:
        data = simplejson.loads(value)
        return simplejson.dumps(data, indent=2, sort_keys=True)
    except:
        return value

@register.filter
@stringfilter
def glext_spec_link(value):
    c = value.split('_', 2)
    return 'http://www.opengl.org/registry/specs/%s/%s.txt' % (c[1], c[2])

@register.filter
@stringfilter
def prettify_gl_title(value):
    if value.startswith('GL_FRAGMENT_PROGRAM_ARB.'):
        return value[24:] + ' (fragment)'
    if value.startswith('GL_VERTEX_PROGRAM_ARB.'):
        return value[22:] + ' (vertex)'
    return value

@register.filter
def dictget(value, key):
    return value.get(key, '')

@register.filter
def sorteditems(value):
    return sorted(value.items(), key = lambda (k, v): k)

@register.filter
def sorteddeviceitems(value):
    return sorted(value.items(), key = lambda (k, v): (k['vendor'], k['renderer'], k['os'], v))

@register.filter
def sortedcpuitems(value):
    return sorted(value.items(), key = lambda (k, v): (k['x86_vendor'], k['x86_model'], k['x86_family'], k['cpu_identifier']))

@register.filter
def cpufreqformat(value):
    return mark_safe("%.2f&nbsp;GHz" % (int(value)/1000000000.0))

@register.filter
def sort(value):
    return sorted(value)

@register.filter
def sortreversed(value):
    return reversed(sorted(value))

@register.filter
def reverse(value):
    return reversed(value)

@register.filter
def format_profile(table):
    cols = set()
    def extract_cols(t):
        for name, row in t.items():
            for n in row:
                cols.add(n)
            if 'children' in row:
                extract_cols(row['children'])
    extract_cols(table)
    if 'children' in cols:
        cols.remove('children')

    if 'msec/frame' in cols:
        cols = ('msec/frame', 'calls/frame', '%/frame', '%/parent', 'mem allocs')
    else:
        cols = sorted(cols)


    out = ['<th>']
    for c in cols:
        out.append(u'<th>%s' % conditional_escape(c))

    def handle(indents, indent, t):
        if 'msec/frame' in cols:
            items = [d[1] for d in sorted((-float(r.get('msec/frame', '')), (n,r)) for (n,r) in t.items())]
        else:
            items = sorted(t.items())

        item_id = 0
        for name, row in items:
            if item_id == len(items) - 1:
                last = True
            else:
                last = False
            item_id += 1

            out.append(u'<tr>')
            out.append(u'<td><span class=treemarker>%s%s─%s╴</span>%s' % (indent, (u'└' if last else u'├'), (u'┬' if 'children' in row else u'─'), conditional_escape(name)))
            for c in cols:
                out.append(u'<td>%s%s' % ('&nbsp;&nbsp;' * indents, conditional_escape(row.get(c, ''))))
            if 'children' in row:
                handle(indents+1, indent+(u'  ' if last else u'│ '), row['children'])
    handle(0, u'', table)

    return mark_safe(u'\n'.join(out))
