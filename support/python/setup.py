#!/usr/bin/env python

import os

from distutils.core import setup

with open(os.path.join("..", os.path.join("..", "VERSION")), "r") as fp:
      version = fp.readline().strip()

setup(name='TraX',
      version=version,
      description='TraX reference implementation wrapper for Python',
      author='Luka Cehovin',
      author_email='luka.cehovin@gmail.com',
      url='https://github.com/votchallenge/trax/',
      packages=['trax']
      )
