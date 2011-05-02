#!/usr/bin/env python

import os

os.environ['DJANGO_SETTINGS_MODULE'] = 'settings'

from userreport import maint
maint.collect_graphics()
