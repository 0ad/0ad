# Fill in this file and save as settings_local.py

DEBUG = True

ADMINS = (
    ('Your Name', 'you@example.com'),
)

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': 'wfgsite',
        'USER': 'wfgsite',
        'PASSWORD': '################',
    }
}

SECRET_KEY = '##################################################'

WFGSITE_ROOT = '/var/####/wfgsite'
