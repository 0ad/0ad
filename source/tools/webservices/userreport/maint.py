from userreport.models import UserReport_hwdetect, GraphicsDevice, GraphicsExtension, GraphicsLimit
from django.db import connection, transaction

def collect_graphics():
    reports = UserReport_hwdetect.objects.filter(data_type = 'hwdetect', data_version__gte = 3)

    print "Gathering data"
    count = 0

    devices = {}
    for report in reports:
        device = report.gl_device_identifier()
        vendor = report.gl_vendor()
        renderer = report.gl_renderer()
        os = report.os()
        driver = report.gl_driver()
        exts = report.gl_extensions()
        limits = report.gl_limits()
        report.clear_cache()

        devices.setdefault(
            (device, vendor, renderer, os, driver, exts, tuple(sorted(limits.items()))),
            set()
        ).add(report.user_id_hash)

        count += 1
        if count % 100 == 0:
            print "%d / %d..." % (count, len(reports))

    print "Saving data"
    count = 0

    cursor = connection.cursor()

    # To get atomic behaviour, construct new tables and then rename them into place at the end
    try:
        cursor.execute('DROP TABLE IF EXISTS userreport_graphicsdevice_temp, userreport_graphicsextension_temp, userreport_graphicslimit_temp')
    except Warning:
        pass # ignore harmless warnings when tables don't exist
    cursor.execute('CREATE TABLE userreport_graphicsdevice_temp LIKE userreport_graphicsdevice')
    cursor.execute('CREATE TABLE userreport_graphicsextension_temp LIKE userreport_graphicsextension')
    cursor.execute('CREATE TABLE userreport_graphicslimit_temp LIKE userreport_graphicslimit')

    for (device, vendor, renderer, os, driver, exts, limits), users in devices.items():
        cursor.execute('''
            INSERT INTO userreport_graphicsdevice_temp (id, device_name, vendor, renderer, os, driver, usercount)
            VALUES (NULL, %s, %s, %s, %s, %s, %s)
        ''', (device, vendor, renderer, os, driver, len(users)))

        device_id = cursor.lastrowid

        if len(exts):
            ext_placeholders = ','.join('(NULL, %s, %s)' for ext in exts)
            ext_values = sum(([device_id, ext] for ext in exts), [])
            cursor.execute('INSERT INTO userreport_graphicsextension_temp (id, device_id, name) VALUES %s' % ext_placeholders, ext_values)

        if len(limits):
            limit_placeholders = ','.join('(NULL, %s, %s, %s)' for limit in limits)
            limit_values = sum(([device_id, name, str(value)] for name,value in limits), [])
            cursor.execute('INSERT INTO userreport_graphicslimit_temp (id, device_id, name, value) VALUES %s' % limit_placeholders, limit_values)

        count += 1
        if count % 100 == 0:
            print "%d / %d..." % (count, len(devices))

    cursor.execute('''
        RENAME TABLE
        userreport_graphicsdevice TO userreport_graphicsdevice_old,
        userreport_graphicsextension TO userreport_graphicsextension_old,
        userreport_graphicslimit TO userreport_graphicslimit_old,
        userreport_graphicsdevice_temp TO userreport_graphicsdevice,
        userreport_graphicsextension_temp TO userreport_graphicsextension,
        userreport_graphicslimit_temp TO userreport_graphicslimit
    ''')
    cursor.execute('''
        DROP TABLE IF EXISTS
        userreport_graphicsdevice_old,
        userreport_graphicsextension_old,
        userreport_graphicslimit_old
    ''')

    transaction.commit_unless_managed()

    print "Finished"
