# http://code.djangoproject.com/attachment/ticket/5908/cycle.py

from django.utils.translation import ungettext, ugettext as _
from django.utils.encoding import force_unicode
from django import template
from django.template import defaultfilters
from django.template import Node, Variable
from django.conf import settings
from itertools import cycle as itertools_cycle

register = template.Library()


   
class SafeCycleNode(Node):
    def __init__(self, cyclevars, variable_name=None):
        self.cyclevars = cyclevars
        self.cycle_iter = itertools_cycle(cyclevars)
        self.variable_name = variable_name

    def render(self, context):
        if context.has_key('forloop'):
            if not context.get(self):
                context[self] = True
                self.cycle_iter = itertools_cycle(self.cyclevars)
        value = self.cycle_iter.next()
        value = Variable(value).resolve(context)
        if self.variable_name:
            context[self.variable_name] = value
        return value



#@register.tag
def safe_cycle(parser, token):
    """
    Cycles among the given strings each time this tag is encountered.

    Within a loop, cycles among the given strings each time through
    the loop::

        {% for o in some_list %}
            <tr class="{% cycle 'row1' 'row2' %}">
                ...
            </tr>
        {% endfor %}

    Outside of a loop, give the values a unique name the first time you call
    it, then use that name each sucessive time through::

            <tr class="{% cycle 'row1' 'row2' 'row3' as rowcolors %}">...</tr>
            <tr class="{% cycle rowcolors %}">...</tr>
            <tr class="{% cycle rowcolors %}">...</tr>

    You can use any number of values, seperated by spaces. Commas can also
    be used to separate values; if a comma is used, the cycle values are
    interpreted as literal strings.
    """

    # Note: This returns the exact same node on each {% cycle name %} call;
    # that is, the node object returned from {% cycle a b c as name %} and the
    # one returned from {% cycle name %} are the exact same object.  This
    # shouldn't cause problems (heh), but if it does, now you know.
    #
    # Ugly hack warning: this stuffs the named template dict into parser so
    # that names are only unique within each template (as opposed to using
    # a global variable, which would make cycle names have to be unique across
    # *all* templates.

    args = token.split_contents()

    if len(args) < 2:
        raise TemplateSyntaxError("'cycle' tag requires at least two arguments")

    if ',' in args[1]:
        # Backwards compatibility: {% cycle a,b %} or {% cycle a,b as foo %}
        # case.
        args[1:2] = ['"%s"' % arg for arg in args[1].split(",")]

    if len(args) == 2:
        # {% cycle foo %} case.
        name = args[1]
        if not hasattr(parser, '_namedCycleNodes'):
            raise TemplateSyntaxError("No named cycles in template."
                                      " '%s' is not defined" % name)
        if not name in parser._namedCycleNodes:
            raise TemplateSyntaxError("Named cycle '%s' does not exist" % name)
        return parser._namedCycleNodes[name]

    if len(args) > 4 and args[-2] == 'as':
        name = args[-1]
        node = SafeCycleNode(args[1:-2], name)
        if not hasattr(parser, '_namedCycleNodes'):
            parser._namedCycleNodes = {}
        parser._namedCycleNodes[name] = node
    else:
        node = SafeCycleNode(args[1:])
    return node
safe_cycle = register.tag(safe_cycle)
