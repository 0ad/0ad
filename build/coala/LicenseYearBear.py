from coalib.bears.LocalBear import LocalBear
from coalib.results.Result import Result
from os.path import getmtime
from re import compile
from datetime import date
# Requires https://pypi.python.org/pypi/svn
from svn.local import LocalClient
from svn.exception import SvnException


class LicenseYearBear(LocalBear):

    """
        Detects a wrong year of a license header
    """

    LANGUAGES = {'C', 'C++'}
    AUTHORS = {'Vladislav Belov'}
    LICENSE = 'GPL-2.0'
    CAN_DETECT = {'Documentation', 'Formatting'}

    def get_last_modification(self, filename):
        client = LocalClient(filename)
        was_modified = False
        for status in client.status():
            if status.type_raw_name == 'modified':
                was_modified = True
                break
        if not was_modified:
            try:
                return client.info()['commit_date'].year
            except SvnException:
                return None
        else:
            return date.today().year

    def run(self, filename, file):
        modification_year = self.get_last_modification(filename)
        if not modification_year:
            return
        license_regexp = compile('/\* Copyright \([cC]?\) ([\d]+) Wildfire Games')

        for line_number, line in enumerate(file):
            match = license_regexp.search(line)
            if not match:
                break

            license_year = match.group(1)
            if not license_year:
                break

            license_year = int(license_year)
            if modification_year == license_year:
                break

            return [Result.from_values(origin=self,
                message='License should have "%d" year instead of "%d"' % (modification_year, license_year),
                file=filename, line=line_number + 1)]

