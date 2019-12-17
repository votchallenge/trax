#!/usr/bin/env python

import os, sys
import setuptools
from distutils.core import setup, Distribution

try:
      from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

      class bdist_wheel(_bdist_wheel):

            def finalize_options(self):
                  _bdist_wheel.finalize_options(self)
                  # Mark us as not a pure python package
                  self.root_is_pure = False

            def get_tag(self):
                  python, abi, plat = _bdist_wheel.get_tag(self)
                  # We don't contain any python source
                  python, abi = 'py2.py3', 'none'
                  return python, abi, plat
except ImportError:
      bdist_wheel = None

try:
      with open(os.path.join("..", os.path.join("..", "VERSION")), "r") as fp:
            version = fp.readline().strip()
except IOError:
      version = os.getenv("TRAX_VERSION", "unknown")

varargs = dict()

platform = os.getenv("TRAX_PYTHON_PLATFORM", sys.platform)

if platform.startswith('linux'):
    trax_library = 'libtrax.so'
elif platform in ['darwin']:
    trax_library = 'libtrax.dynlib'
elif platform in ['win32', "win64"]:
    trax_library = 'trax.dll'

if os.path.isfile(os.path.join("trax", trax_library)):
      varargs["package_data"] = {"trax" : [trax_library]}
      varargs["cmdclass"] = {'bdist_wheel': bdist_wheel}

setup(name='trax',
      version=version,
      description='TraX reference implementation wrapper for Python',
      author='Luka Cehovin',
      author_email='luka.cehovin@gmail.com',
      url='https://github.com/votchallenge/trax/',
      packages=['trax'],
      setup_requires=['wheel'],
      install_requires=["numpy>=1.16",
            "opencv-python>=4.0"],
      **varargs
      )
