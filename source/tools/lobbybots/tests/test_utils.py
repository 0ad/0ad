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

"""Tests for utility functions."""

from unittest import TestCase

from hypothesis import given
from hypothesis import strategies as st

from xpartamupp.utils import LimitedSizeDict


class TestLimitedSizeDict(TestCase):
    """Test limited size dict."""

    @given(st.integers(min_value=2, max_value=2**10))
    def test_max_items(self, size_limit):
        """Test max items of dicts.

        Test that the dict doesn't grow indefinitely and that the
        oldest entries are removed first.
        """
        test_dict = LimitedSizeDict(size_limit=size_limit)
        for i in range(size_limit):
            test_dict[i] = i
        self.assertEqual(size_limit, len(test_dict))
        test_dict[size_limit + 1] = size_limit + 1
        self.assertEqual(size_limit, len(test_dict))
        self.assertFalse(0 in test_dict.values())
        self.assertTrue(1 in test_dict.values())
        self.assertTrue(size_limit + 1 in test_dict.values())
