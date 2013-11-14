#!/usr/bin/python
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# PidDataTest.py
# Copyright (C) 2013 Simon Newton

"""Test to check we can load the Pid data in Python."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import os
import sys
import unittest
from ola import PidStore

class PidDataTest(unittest.TestCase):

  def testLoad(self):
    store = PidStore.GetStore(os.environ['PIDDATA'])
    self.assertIsNotNone(store);

    pids = store.Pids()
    self.assertNotEqual(0, len(pids))

if __name__ == '__main__':
  unittest.main()