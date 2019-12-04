#!/usr/bin/env python

import os, sys
import setuptools
from distutils.core import setup, Distribution

class BinaryDistribution(Distribution):
      def has_ext_modules(self):
            return True

try:
      with open(os.path.join("..", os.path.join("..", "VERSION")), "r") as fp:
            version = fp.readline().strip()
except IOError:
      version = "unknown"

varargs = dict()

if sys.platform.startswith('linux'):
    trax_library = 'libtrax.so'
elif sys.platform in ['darwin']:
    trax_library = 'libtrax.dynlib'
elif sys.platform in ['win32']:
    trax_library = 'trax.dll'

if os.path.isfile(os.path.join("trax", trax_library)):
      varargs["package_data"] = {"trax" : [trax_library]}
      varargs["distclass"] = BinaryDistribution
      varargs["ext_modules"] = []

setup(name='TraX',
      version=version,
      description='TraX reference implementation wrapper for Python',
      author='Luka Cehovin',
      author_email='luka.cehovin@gmail.com',
      url='https://github.com/votchallenge/trax/',
      packages=['trax'],
      setup_requires=['wheel'],
      requires=[
            "numpy>=1.16",
            "opencv-python>=4.0"]
      **varargs
      )
