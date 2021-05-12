# Copyright (C) 2021 Wildfire Games.
# This file is part of 0 A.D.
#
# 0 A.D. is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# 0 A.D. is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.

"""Collection of utility functions used by the XMPP-bots."""

from collections import OrderedDict


class LimitedSizeDict(OrderedDict):
    """Dictionary with limited size and FIFO characteristics."""

    def __init__(self, *args, **kwargs):
        """Initialize the dictionary.

        Set the limit to which size the dict should be able to grow.
        """
        self.size_limit = kwargs.pop('size_limit', None)
        OrderedDict.__init__(self, *args, **kwargs)
        self._check_size_limit()

    def __setitem__(self, key, value):  # pylint: disable=signature-differs
        """Overwrite default method to add size limit check."""
        OrderedDict.__setitem__(self, key, value)
        self._check_size_limit()

    def _check_size_limit(self):
        """Ensure dict is not larger than the size limit.

        Compares the current size of the dict with the size limit and
        removes items from the dict until the size is equal the size
        limit.
        """
        if self.size_limit:
            while len(self) > self.size_limit:
                self.popitem(last=False)
