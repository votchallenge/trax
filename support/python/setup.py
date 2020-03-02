#!/usr/bin/env python

import os, sys, glob
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

root = os.path.abspath(os.path.dirname(__file__))
platform = os.getenv("TRAX_PYTHON_PLATFORM", sys.platform)

if platform.startswith('linux'):
    library_prefix = 'lib'
    library_suffix = '.so'
elif platform in ['darwin']:
    library_prefix = 'lib'
    library_suffix = '.dylib'
elif platform.startswith('win'):
    library_prefix = ''
    library_suffix = '.dll'

class build_ext_ctypes(build_ext):

    def build_extension(self, ext):
        self._ctypes = isinstance(ext, CTypes)
        return super().build_extension(ext)

    def get_export_symbols(self, ext):
        if self._ctypes:
            return ext.export_symbols
        return super().get_export_symbols(ext)

    def get_ext_filename(self, ext_name):
        if self._ctypes:
            return library_prefix + ext_name + library_suffix
        return super().get_ext_filename(ext_name)

class CTypes(Extension): 
    pass

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
    with open(os.path.join(root, "VERSION"), encoding='utf-8') as fp:
        VERSION = fp.readline().strip()

except IOError:
    VERSION = os.getenv("TRAX_VERSION", "unknown")

try:
    with open(os.path.join(root, 'README.md'), encoding='utf-8') as f:
        long_description = f.read()

except IOError:
    long_description = ""


varargs = dict()

if os.path.isfile(os.path.join("trax", library_prefix + "trax" + library_suffix)):
    varargs["package_data"] = {"trax" : [library_prefix + "trax" + library_suffix]}
    varargs["cmdclass"] = {'bdist_wheel': bdist_wheel}
    varargs["setup_requires"] = ['wheel']
elif os.path.isfile(os.path.join("trax", "trax.c")):
    sources = glob.glob("trax/*.c") + glob.glob("trax/*.cpp")
    varargs["ext_modules"] = [CTypes("trax.trax", sources=sources, define_macros=[("trax_EXPORTS", "1")])]
    varargs["cmdclass"] = {'build_ext': build_ext_ctypes}

setup(name='vot-trax',
    version=VERSION,
    description='TraX protocol reference implementation wrapper for Python',
    author='Luka Cehovin Zajc',
    author_email='luka.cehovin@gmail.com',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/votchallenge/trax/',
    packages=['trax'],
    install_requires=["numpy>=1.16", "six"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: BSD License",
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Science/Research",
    ],
    **varargs
)