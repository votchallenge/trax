r"""Wrapper for trax.h

Generated with:
/home/lukacu/.virtualenvs/vot/bin/ctypesgen -l libtrax ../include/trax.h -o _ctypes.py

Do not modify this file.
"""

__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types




# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception:  # pylint: disable=broad-except
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "%sd.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)

del loaderclass

# End loader

add_library_search_dirs([os.path.dirname(__file__)])

# Begin libraries
_libs["trax"] = load_library("trax")

# 1 libraries
# End libraries

# No modules

__off_t = c_long# /usr/include/x86_64-linux-gnu/bits/types.h: 152

__off64_t = c_long# /usr/include/x86_64-linux-gnu/bits/types.h: 153

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 36
class struct__IO_marker(Structure):
    pass

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 37
class struct__IO_codecvt(Structure):
    pass

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 38
class struct__IO_wide_data(Structure):
    pass

_IO_lock_t = None# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 43

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 137
class struct_trax_image(Structure):
    pass

struct_trax_image.__slots__ = [
    'type',
    'width',
    'height',
    'format',
    'data',
]
struct_trax_image._fields_ = [
    ('type', c_short),
    ('width', c_int),
    ('height', c_int),
    ('format', c_int),
    ('data', POINTER(ctypes.c_char)),
]

trax_image = struct_trax_image# /home/lukacu/Checkouts/vot/trax/include/trax.h: 137

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 149
class struct_trax_properties(Structure):
    pass

trax_properties = struct_trax_properties# /home/lukacu/Checkouts/vot/trax/include/trax.h: 149

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 159
class struct_trax_object_list(Structure):
    pass

struct_trax_object_list.__slots__ = [
    'size',
    'regions',
    'properties',
]
struct_trax_object_list._fields_ = [
    ('size', c_int),
    ('regions', POINTER(ctypes.c_void_p)),
    ('properties', POINTER(POINTER(trax_properties))),
]

trax_object_list = struct_trax_object_list# /home/lukacu/Checkouts/vot/trax/include/trax.h: 159

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 168
class struct_trax_bounds(Structure):
    pass

struct_trax_bounds.__slots__ = [
    'top',
    'bottom',
    'left',
    'right',
]
struct_trax_bounds._fields_ = [
    ('top', c_float),
    ('bottom', c_float),
    ('left', c_float),
    ('right', c_float),
]

trax_bounds = struct_trax_bounds# /home/lukacu/Checkouts/vot/trax/include/trax.h: 168

trax_logger = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int32, ctypes.c_void_p)
trax_enumerator = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 181
class struct_trax_logging(Structure):
    pass

struct_trax_logging.__slots__ = [
    'flags',
    'callback',
    'data',
]
struct_trax_logging._fields_ = [
    ('flags', c_int),
    ('callback', trax_logger),
    ('data', POINTER(None)),
]

trax_logging = struct_trax_logging# /home/lukacu/Checkouts/vot/trax/include/trax.h: 181

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 195
class struct_trax_metadata(Structure):
    pass

struct_trax_metadata.__slots__ = [
    'format_region',
    'format_image',
    'channels',
    'flags',
    'tracker_name',
    'tracker_description',
    'tracker_family',
    'custom',
]
struct_trax_metadata._fields_ = [
    ('format_region', c_int),
    ('format_image', c_int),
    ('channels', c_int),
    ('flags', c_int),
    ('tracker_name', ctypes.c_char_p),
    ('tracker_description', ctypes.c_char_p),
    ('tracker_family', ctypes.c_char_p),
    ('custom', POINTER(trax_properties)),
]

trax_metadata = struct_trax_metadata# /home/lukacu/Checkouts/vot/trax/include/trax.h: 195

trax_configuration = trax_metadata# /home/lukacu/Checkouts/vot/trax/include/trax.h: 197

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 210
class struct_trax_handle(Structure):
    pass

struct_trax_handle.__slots__ = [
    'flags',
    'version',
    'stream',
    'logging',
    'metadata',
    'error',
    'objects',
]
struct_trax_handle._fields_ = [
    ('flags', c_int),
    ('version', c_int),
    ('stream', POINTER(None)),
    ('logging', trax_logging),
    ('metadata', POINTER(trax_metadata)),
    ('error', ctypes.c_char_p),
    ('objects', c_int),
]

trax_handle = struct_trax_handle# /home/lukacu/Checkouts/vot/trax/include/trax.h: 210

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 217
class struct_trax_image_list(Structure):
    pass

struct_trax_image_list.__slots__ = [
    'images',
]
struct_trax_image_list._fields_ = [
    ('images', POINTER(trax_image) * int(3)),
]

trax_image_list = struct_trax_image_list# /home/lukacu/Checkouts/vot/trax/include/trax.h: 217

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 219
try:
    trax_no_log = (trax_logging).in_dll(_libs["trax"], "trax_no_log")
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 221
try:
    trax_no_bounds = (trax_bounds).in_dll(_libs["trax"], "trax_no_bounds")
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 226
if _libs["trax"].has("trax_version", "cdecl"):
    trax_version = _libs["trax"].get("trax_version", "cdecl")
    trax_version.argtypes = []
    trax_version.restype = ctypes.c_char_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 231
if _libs["trax"].has("trax_metadata_create", "cdecl"):
    trax_metadata_create = _libs["trax"].get("trax_metadata_create", "cdecl")
    trax_metadata_create.argtypes = [c_int, c_int, c_int, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, c_int]
    trax_metadata_create.restype = POINTER(trax_metadata)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 237
if _libs["trax"].has("trax_metadata_release", "cdecl"):
    trax_metadata_release = _libs["trax"].get("trax_metadata_release", "cdecl")
    trax_metadata_release.argtypes = [POINTER(POINTER(trax_metadata))]
    trax_metadata_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 242
if _libs["trax"].has("trax_logger_setup", "cdecl"):
    trax_logger_setup = _libs["trax"].get("trax_logger_setup", "cdecl")
    trax_logger_setup.argtypes = [trax_logger, ctypes.c_void_p, c_int]
    trax_logger_setup.restype = trax_logging


# /home/lukacu/Checkouts/vot/trax/include/trax.h: 252
if _libs["trax"].has("trax_client_setup_file", "cdecl"):
    trax_client_setup_file = _libs["trax"].get("trax_client_setup_file", "cdecl")
    trax_client_setup_file.argtypes = [c_int, c_int, trax_logging]
    trax_client_setup_file.restype = POINTER(trax_handle)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 257
if _libs["trax"].has("trax_client_setup_socket", "cdecl"):
    trax_client_setup_socket = _libs["trax"].get("trax_client_setup_socket", "cdecl")
    trax_client_setup_socket.argtypes = [c_int, c_int, trax_logging]
    trax_client_setup_socket.restype = POINTER(trax_handle)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 262
if _libs["trax"].has("trax_client_wait", "cdecl"):
    trax_client_wait = _libs["trax"].get("trax_client_wait", "cdecl")
    trax_client_wait.argtypes = [POINTER(trax_handle), POINTER(POINTER(trax_object_list)), POINTER(trax_properties)]
    trax_client_wait.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 267
if _libs["trax"].has("trax_client_initialize", "cdecl"):
    trax_client_initialize = _libs["trax"].get("trax_client_initialize", "cdecl")
    trax_client_initialize.argtypes = [POINTER(trax_handle), POINTER(trax_image_list), POINTER(trax_object_list), POINTER(trax_properties)]
    trax_client_initialize.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 272
if _libs["trax"].has("trax_client_frame", "cdecl"):
    trax_client_frame = _libs["trax"].get("trax_client_frame", "cdecl")
    trax_client_frame.argtypes = [POINTER(trax_handle), POINTER(trax_image_list), POINTER(trax_object_list), POINTER(trax_properties)]
    trax_client_frame.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 277
if _libs["trax"].has("trax_server_setup_v", "cdecl"):
    trax_server_setup_v = _libs["trax"].get("trax_server_setup_v", "cdecl")
    trax_server_setup_v.argtypes = [POINTER(trax_metadata), trax_logging, c_int]
    trax_server_setup_v.restype = POINTER(trax_handle)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 282
if _libs["trax"].has("trax_server_setup_file_v", "cdecl"):
    trax_server_setup_file_v = _libs["trax"].get("trax_server_setup_file_v", "cdecl")
    trax_server_setup_file_v.argtypes = [POINTER(trax_metadata), c_int, c_int, trax_logging, c_int]
    trax_server_setup_file_v.restype = POINTER(trax_handle)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 287
if _libs["trax"].has("trax_server_wait_sot", "cdecl"):
    trax_server_wait_sot = _libs["trax"].get("trax_server_wait_sot", "cdecl")
    trax_server_wait_sot.argtypes = [POINTER(trax_handle), POINTER(POINTER(trax_image_list)), POINTER(ctypes.c_void_p), POINTER(trax_properties)]
    trax_server_wait_sot.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 292
if _libs["trax"].has("trax_server_reply_sot", "cdecl"):
    trax_server_reply_sot = _libs["trax"].get("trax_server_reply_sot", "cdecl")
    trax_server_reply_sot.argtypes = [POINTER(trax_handle), ctypes.c_void_p, POINTER(trax_properties)]
    trax_server_reply_sot.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 297
if _libs["trax"].has("trax_server_wait_mot", "cdecl"):
    trax_server_wait_mot = _libs["trax"].get("trax_server_wait_mot", "cdecl")
    trax_server_wait_mot.argtypes = [POINTER(trax_handle), POINTER(POINTER(trax_image_list)), POINTER(POINTER(trax_object_list)), POINTER(trax_properties)]
    trax_server_wait_mot.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 302
if _libs["trax"].has("trax_server_reply_mot", "cdecl"):
    trax_server_reply_mot = _libs["trax"].get("trax_server_reply_mot", "cdecl")
    trax_server_reply_mot.argtypes = [POINTER(trax_handle), POINTER(trax_object_list)]
    trax_server_reply_mot.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 307
if _libs["trax"].has("trax_terminate", "cdecl"):
    trax_terminate = _libs["trax"].get("trax_terminate", "cdecl")
    trax_terminate.argtypes = [POINTER(trax_handle), ctypes.c_char_p]
    trax_terminate.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 312
if _libs["trax"].has("trax_get_error", "cdecl"):
    trax_get_error = _libs["trax"].get("trax_get_error", "cdecl")
    trax_get_error.argtypes = [POINTER(trax_handle)]
    trax_get_error.restype = ctypes.c_char_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 318
if _libs["trax"].has("trax_is_alive", "cdecl"):
    trax_is_alive = _libs["trax"].get("trax_is_alive", "cdecl")
    trax_is_alive.argtypes = [POINTER(trax_handle)]
    trax_is_alive.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 324
if _libs["trax"].has("trax_cleanup", "cdecl"):
    trax_cleanup = _libs["trax"].get("trax_cleanup", "cdecl")
    trax_cleanup.argtypes = [POINTER(POINTER(trax_handle))]
    trax_cleanup.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 329
if _libs["trax"].has("trax_set_parameter", "cdecl"):
    trax_set_parameter = _libs["trax"].get("trax_set_parameter", "cdecl")
    trax_set_parameter.argtypes = [POINTER(trax_handle), c_int, c_int]
    trax_set_parameter.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 334
if _libs["trax"].has("trax_get_parameter", "cdecl"):
    trax_get_parameter = _libs["trax"].get("trax_get_parameter", "cdecl")
    trax_get_parameter.argtypes = [POINTER(trax_handle), c_int, POINTER(c_int)]
    trax_get_parameter.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 339
if _libs["trax"].has("trax_image_release", "cdecl"):
    trax_image_release = _libs["trax"].get("trax_image_release", "cdecl")
    trax_image_release.argtypes = [POINTER(POINTER(trax_image))]
    trax_image_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 344
if _libs["trax"].has("trax_image_create_path", "cdecl"):
    trax_image_create_path = _libs["trax"].get("trax_image_create_path", "cdecl")
    trax_image_create_path.argtypes = [ctypes.c_char_p]
    trax_image_create_path.restype = POINTER(trax_image)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 349
if _libs["trax"].has("trax_image_create_url", "cdecl"):
    trax_image_create_url = _libs["trax"].get("trax_image_create_url", "cdecl")
    trax_image_create_url.argtypes = [ctypes.c_char_p]
    trax_image_create_url.restype = POINTER(trax_image)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 354
if _libs["trax"].has("trax_image_create_memory", "cdecl"):
    trax_image_create_memory = _libs["trax"].get("trax_image_create_memory", "cdecl")
    trax_image_create_memory.argtypes = [c_int, c_int, c_int]
    trax_image_create_memory.restype = POINTER(trax_image)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 359
if _libs["trax"].has("trax_image_create_buffer", "cdecl"):
    trax_image_create_buffer = _libs["trax"].get("trax_image_create_buffer", "cdecl")
    trax_image_create_buffer.argtypes = [c_int, ctypes.c_char_p]
    trax_image_create_buffer.restype = POINTER(trax_image)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 364
if _libs["trax"].has("trax_image_get_type", "cdecl"):
    trax_image_get_type = _libs["trax"].get("trax_image_get_type", "cdecl")
    trax_image_get_type.argtypes = [POINTER(trax_image)]
    trax_image_get_type.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 370
if _libs["trax"].has("trax_image_get_path", "cdecl"):
    trax_image_get_path = _libs["trax"].get("trax_image_get_path", "cdecl")
    trax_image_get_path.argtypes = [POINTER(trax_image)]
    trax_image_get_path.restype = ctypes.c_char_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 376
if _libs["trax"].has("trax_image_get_url", "cdecl"):
    trax_image_get_url = _libs["trax"].get("trax_image_get_url", "cdecl")
    trax_image_get_url.argtypes = [POINTER(trax_image)]
    trax_image_get_url.restype = ctypes.c_char_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 381
if _libs["trax"].has("trax_image_get_memory_header", "cdecl"):
    trax_image_get_memory_header = _libs["trax"].get("trax_image_get_memory_header", "cdecl")
    trax_image_get_memory_header.argtypes = [POINTER(trax_image), POINTER(c_int), POINTER(c_int), POINTER(c_int)]
    trax_image_get_memory_header.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 386
if _libs["trax"].has("trax_image_write_memory_row", "cdecl"):
    trax_image_write_memory_row = _libs["trax"].get("trax_image_write_memory_row", "cdecl")
    trax_image_write_memory_row.argtypes = [POINTER(trax_image), c_int]
    trax_image_write_memory_row.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 391
if _libs["trax"].has("trax_image_get_memory_row", "cdecl"):
    trax_image_get_memory_row = _libs["trax"].get("trax_image_get_memory_row", "cdecl")
    trax_image_get_memory_row.argtypes = [POINTER(trax_image), c_int]
    trax_image_get_memory_row.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 397
if _libs["trax"].has("trax_image_get_buffer", "cdecl"):
    trax_image_get_buffer = _libs["trax"].get("trax_image_get_buffer", "cdecl")
    trax_image_get_buffer.argtypes = [POINTER(trax_image), POINTER(c_int), POINTER(c_int)]
    trax_image_get_buffer.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 402
if _libs["trax"].has("trax_region_release", "cdecl"):
    trax_region_release = _libs["trax"].get("trax_region_release", "cdecl")
    trax_region_release.argtypes = [POINTER(ctypes.c_void_p)]
    trax_region_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 407
if _libs["trax"].has("trax_region_get_type", "cdecl"):
    trax_region_get_type = _libs["trax"].get("trax_region_get_type", "cdecl")
    trax_region_get_type.argtypes = [ctypes.c_void_p]
    trax_region_get_type.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 412
if _libs["trax"].has("trax_region_create_special", "cdecl"):
    trax_region_create_special = _libs["trax"].get("trax_region_create_special", "cdecl")
    trax_region_create_special.argtypes = [c_int]
    trax_region_create_special.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 417
if _libs["trax"].has("trax_region_set_special", "cdecl"):
    trax_region_set_special = _libs["trax"].get("trax_region_set_special", "cdecl")
    trax_region_set_special.argtypes = [ctypes.c_void_p, c_int]
    trax_region_set_special.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 422
if _libs["trax"].has("trax_region_get_special", "cdecl"):
    trax_region_get_special = _libs["trax"].get("trax_region_get_special", "cdecl")
    trax_region_get_special.argtypes = [ctypes.c_void_p]
    trax_region_get_special.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 427
if _libs["trax"].has("trax_region_create_rectangle", "cdecl"):
    trax_region_create_rectangle = _libs["trax"].get("trax_region_create_rectangle", "cdecl")
    trax_region_create_rectangle.argtypes = [c_float, c_float, c_float, c_float]
    trax_region_create_rectangle.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 432
if _libs["trax"].has("trax_region_set_rectangle", "cdecl"):
    trax_region_set_rectangle = _libs["trax"].get("trax_region_set_rectangle", "cdecl")
    trax_region_set_rectangle.argtypes = [ctypes.c_void_p, c_float, c_float, c_float, c_float]
    trax_region_set_rectangle.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 437
if _libs["trax"].has("trax_region_get_rectangle", "cdecl"):
    trax_region_get_rectangle = _libs["trax"].get("trax_region_get_rectangle", "cdecl")
    trax_region_get_rectangle.argtypes = [ctypes.c_void_p, POINTER(c_float), POINTER(c_float), POINTER(c_float), POINTER(c_float)]
    trax_region_get_rectangle.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 443
if _libs["trax"].has("trax_region_create_polygon", "cdecl"):
    trax_region_create_polygon = _libs["trax"].get("trax_region_create_polygon", "cdecl")
    trax_region_create_polygon.argtypes = [c_int]
    trax_region_create_polygon.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 448
if _libs["trax"].has("trax_region_set_polygon_point", "cdecl"):
    trax_region_set_polygon_point = _libs["trax"].get("trax_region_set_polygon_point", "cdecl")
    trax_region_set_polygon_point.argtypes = [ctypes.c_void_p, c_int, c_float, c_float]
    trax_region_set_polygon_point.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 453
if _libs["trax"].has("trax_region_get_polygon_point", "cdecl"):
    trax_region_get_polygon_point = _libs["trax"].get("trax_region_get_polygon_point", "cdecl")
    trax_region_get_polygon_point.argtypes = [ctypes.c_void_p, c_int, POINTER(c_float), POINTER(c_float)]
    trax_region_get_polygon_point.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 458
if _libs["trax"].has("trax_region_get_polygon_count", "cdecl"):
    trax_region_get_polygon_count = _libs["trax"].get("trax_region_get_polygon_count", "cdecl")
    trax_region_get_polygon_count.argtypes = [ctypes.c_void_p]
    trax_region_get_polygon_count.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 463
if _libs["trax"].has("trax_region_create_mask", "cdecl"):
    trax_region_create_mask = _libs["trax"].get("trax_region_create_mask", "cdecl")
    trax_region_create_mask.argtypes = [c_int, c_int, c_int, c_int]
    trax_region_create_mask.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 468
if _libs["trax"].has("trax_region_get_mask_header", "cdecl"):
    trax_region_get_mask_header = _libs["trax"].get("trax_region_get_mask_header", "cdecl")
    trax_region_get_mask_header.argtypes = [ctypes.c_void_p, POINTER(c_int), POINTER(c_int), POINTER(c_int), POINTER(c_int)]
    trax_region_get_mask_header.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 473
if _libs["trax"].has("trax_region_write_mask_row", "cdecl"):
    trax_region_write_mask_row = _libs["trax"].get("trax_region_write_mask_row", "cdecl")
    trax_region_write_mask_row.argtypes = [ctypes.c_void_p, c_int]
    trax_region_write_mask_row.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 478
if _libs["trax"].has("trax_region_get_mask_row", "cdecl"):
    trax_region_get_mask_row = _libs["trax"].get("trax_region_get_mask_row", "cdecl")
    trax_region_get_mask_row.argtypes = [ctypes.c_void_p, c_int]
    trax_region_get_mask_row.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 483
if _libs["trax"].has("trax_region_bounds", "cdecl"):
    trax_region_bounds = _libs["trax"].get("trax_region_bounds", "cdecl")
    trax_region_bounds.argtypes = [ctypes.c_void_p]
    trax_region_bounds.restype = trax_bounds

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 488
if _libs["trax"].has("trax_region_contains", "cdecl"):
    trax_region_contains = _libs["trax"].get("trax_region_contains", "cdecl")
    trax_region_contains.argtypes = [ctypes.c_void_p, c_float, c_float]
    trax_region_contains.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 493
if _libs["trax"].has("trax_region_clone", "cdecl"):
    trax_region_clone = _libs["trax"].get("trax_region_clone", "cdecl")
    trax_region_clone.argtypes = [ctypes.c_void_p]
    trax_region_clone.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 498
if _libs["trax"].has("trax_region_convert", "cdecl"):
    trax_region_convert = _libs["trax"].get("trax_region_convert", "cdecl")
    trax_region_convert.argtypes = [ctypes.c_void_p, c_int]
    trax_region_convert.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 503
if _libs["trax"].has("trax_region_overlap", "cdecl"):
    trax_region_overlap = _libs["trax"].get("trax_region_overlap", "cdecl")
    trax_region_overlap.argtypes = [ctypes.c_void_p, ctypes.c_void_p, trax_bounds]
    trax_region_overlap.restype = c_float

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 508
if _libs["trax"].has("trax_region_encode", "cdecl"):
    trax_region_encode = _libs["trax"].get("trax_region_encode", "cdecl")
    trax_region_encode.argtypes = [ctypes.c_void_p]
    trax_region_encode.restype = POINTER(ctypes.c_char)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 513
if _libs["trax"].has("trax_region_decode", "cdecl"):
    trax_region_decode = _libs["trax"].get("trax_region_decode", "cdecl")
    trax_region_decode.argtypes = [ctypes.c_char_p]
    trax_region_decode.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 518
if _libs["trax"].has("trax_object_list_create", "cdecl"):
    trax_object_list_create = _libs["trax"].get("trax_object_list_create", "cdecl")
    trax_object_list_create.argtypes = [c_int]
    trax_object_list_create.restype = POINTER(trax_object_list)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 523
if _libs["trax"].has("trax_object_list_release", "cdecl"):
    trax_object_list_release = _libs["trax"].get("trax_object_list_release", "cdecl")
    trax_object_list_release.argtypes = [POINTER(POINTER(trax_object_list))]
    trax_object_list_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 528
if _libs["trax"].has("trax_object_list_set", "cdecl"):
    trax_object_list_set = _libs["trax"].get("trax_object_list_set", "cdecl")
    trax_object_list_set.argtypes = [POINTER(trax_object_list), c_int, ctypes.c_void_p]
    trax_object_list_set.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 533
if _libs["trax"].has("trax_object_list_get", "cdecl"):
    trax_object_list_get = _libs["trax"].get("trax_object_list_get", "cdecl")
    trax_object_list_get.argtypes = [POINTER(trax_object_list), c_int]
    trax_object_list_get.restype = ctypes.c_void_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 538
if _libs["trax"].has("trax_object_list_properties", "cdecl"):
    trax_object_list_properties = _libs["trax"].get("trax_object_list_properties", "cdecl")
    trax_object_list_properties.argtypes = [POINTER(trax_object_list), c_int]
    trax_object_list_properties.restype = POINTER(trax_properties)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 543
if _libs["trax"].has("trax_object_list_count", "cdecl"):
    trax_object_list_count = _libs["trax"].get("trax_object_list_count", "cdecl")
    trax_object_list_count.argtypes = [POINTER(trax_object_list)]
    trax_object_list_count.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 548
if _libs["trax"].has("trax_object_list_append", "cdecl"):
    trax_object_list_append = _libs["trax"].get("trax_object_list_append", "cdecl")
    trax_object_list_append.argtypes = [POINTER(trax_object_list), POINTER(trax_object_list)]
    trax_object_list_append.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 553
if _libs["trax"].has("trax_properties_release", "cdecl"):
    trax_properties_release = _libs["trax"].get("trax_properties_release", "cdecl")
    trax_properties_release.argtypes = [POINTER(POINTER(trax_properties))]
    trax_properties_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 558
if _libs["trax"].has("trax_properties_clear", "cdecl"):
    trax_properties_clear = _libs["trax"].get("trax_properties_clear", "cdecl")
    trax_properties_clear.argtypes = [POINTER(trax_properties)]
    trax_properties_clear.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 563
if _libs["trax"].has("trax_properties_create", "cdecl"):
    trax_properties_create = _libs["trax"].get("trax_properties_create", "cdecl")
    trax_properties_create.argtypes = []
    trax_properties_create.restype = POINTER(trax_properties)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 568
if _libs["trax"].has("trax_properties_copy", "cdecl"):
    trax_properties_copy = _libs["trax"].get("trax_properties_copy", "cdecl")
    trax_properties_copy.argtypes = [POINTER(trax_properties)]
    trax_properties_copy.restype = POINTER(trax_properties)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 573
if _libs["trax"].has("trax_properties_has", "cdecl"):
    trax_properties_has = _libs["trax"].get("trax_properties_has", "cdecl")
    trax_properties_has.argtypes = [POINTER(trax_properties), ctypes.c_char_p]
    trax_properties_has.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 578
if _libs["trax"].has("trax_properties_set", "cdecl"):
    trax_properties_set = _libs["trax"].get("trax_properties_set", "cdecl")
    trax_properties_set.argtypes = [POINTER(trax_properties), ctypes.c_char_p, ctypes.c_char_p]
    trax_properties_set.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 583
if _libs["trax"].has("trax_properties_set_int", "cdecl"):
    trax_properties_set_int = _libs["trax"].get("trax_properties_set_int", "cdecl")
    trax_properties_set_int.argtypes = [POINTER(trax_properties), ctypes.c_char_p, c_int]
    trax_properties_set_int.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 588
if _libs["trax"].has("trax_properties_set_float", "cdecl"):
    trax_properties_set_float = _libs["trax"].get("trax_properties_set_float", "cdecl")
    trax_properties_set_float.argtypes = [POINTER(trax_properties), ctypes.c_char_p, c_float]
    trax_properties_set_float.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 594
if _libs["trax"].has("trax_properties_get", "cdecl"):
    trax_properties_get = _libs["trax"].get("trax_properties_get", "cdecl")
    trax_properties_get.argtypes = [POINTER(trax_properties), ctypes.c_char_p]
    trax_properties_get.restype = ctypes.c_char_p

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 600
if _libs["trax"].has("trax_properties_get_int", "cdecl"):
    trax_properties_get_int = _libs["trax"].get("trax_properties_get_int", "cdecl")
    trax_properties_get_int.argtypes = [POINTER(trax_properties), ctypes.c_char_p, c_int]
    trax_properties_get_int.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 606
if _libs["trax"].has("trax_properties_get_float", "cdecl"):
    trax_properties_get_float = _libs["trax"].get("trax_properties_get_float", "cdecl")
    trax_properties_get_float.argtypes = [POINTER(trax_properties), ctypes.c_char_p, c_float]
    trax_properties_get_float.restype = c_float

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 611
if _libs["trax"].has("trax_properties_count", "cdecl"):
    trax_properties_count = _libs["trax"].get("trax_properties_count", "cdecl")
    trax_properties_count.argtypes = [POINTER(trax_properties)]
    trax_properties_count.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 617
if _libs["trax"].has("trax_properties_enumerate", "cdecl"):
    trax_properties_enumerate = _libs["trax"].get("trax_properties_enumerate", "cdecl")
    trax_properties_enumerate.argtypes = [POINTER(struct_trax_properties), trax_enumerator, ctypes.py_object]
    trax_properties_enumerate.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 622
if _libs["trax"].has("trax_properties_append", "cdecl"):
    trax_properties_append = _libs["trax"].get("trax_properties_append", "cdecl")
    trax_properties_append.argtypes = [POINTER(trax_properties), POINTER(trax_properties), c_int]
    trax_properties_append.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 627
if _libs["trax"].has("trax_image_list_create", "cdecl"):
    trax_image_list_create = _libs["trax"].get("trax_image_list_create", "cdecl")
    trax_image_list_create.argtypes = []
    trax_image_list_create.restype = POINTER(trax_image_list)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 632
if _libs["trax"].has("trax_image_list_release", "cdecl"):
    trax_image_list_release = _libs["trax"].get("trax_image_list_release", "cdecl")
    trax_image_list_release.argtypes = [POINTER(POINTER(trax_image_list))]
    trax_image_list_release.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 637
if _libs["trax"].has("trax_image_list_clear", "cdecl"):
    trax_image_list_clear = _libs["trax"].get("trax_image_list_clear", "cdecl")
    trax_image_list_clear.argtypes = [POINTER(trax_image_list)]
    trax_image_list_clear.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 642
if _libs["trax"].has("trax_image_list_get", "cdecl"):
    trax_image_list_get = _libs["trax"].get("trax_image_list_get", "cdecl")
    trax_image_list_get.argtypes = [POINTER(trax_image_list), c_int]
    trax_image_list_get.restype = POINTER(trax_image)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 647
if _libs["trax"].has("trax_image_list_set", "cdecl"):
    trax_image_list_set = _libs["trax"].get("trax_image_list_set", "cdecl")
    trax_image_list_set.argtypes = [POINTER(trax_image_list), POINTER(trax_image), c_int]
    trax_image_list_set.restype = None

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 652
if _libs["trax"].has("trax_image_list_count", "cdecl"):
    trax_image_list_count = _libs["trax"].get("trax_image_list_count", "cdecl")
    trax_image_list_count.argtypes = [c_int]
    trax_image_list_count.restype = c_int

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 39
try:
    TRAX_NO_LOG = (-1)
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 42
try:
    TRAX_VERSION = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 44
try:
    TRAX_ERROR = (-1)
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 45
try:
    TRAX_OK = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 46
try:
    TRAX_HELLO = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 47
try:
    TRAX_INITIALIZE = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 48
try:
    TRAX_FRAME = 3
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 49
try:
    TRAX_QUIT = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 50
try:
    TRAX_STATE = 5
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 54
try:
    TRAX_IMAGE_EMPTY = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 55
try:
    TRAX_IMAGE_PATH = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 56
try:
    TRAX_IMAGE_URL = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 57
try:
    TRAX_IMAGE_MEMORY = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 58
try:
    TRAX_IMAGE_BUFFER = 8
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 60
try:
    TRAX_IMAGE_ANY = (((TRAX_IMAGE_PATH | TRAX_IMAGE_URL) | TRAX_IMAGE_MEMORY) | TRAX_IMAGE_BUFFER)
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 62
try:
    TRAX_IMAGE_BUFFER_ILLEGAL = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 63
try:
    TRAX_IMAGE_BUFFER_PNG = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 64
try:
    TRAX_IMAGE_BUFFER_JPEG = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 66
try:
    TRAX_IMAGE_MEMORY_ILLEGAL = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 67
try:
    TRAX_IMAGE_MEMORY_GRAY8 = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 68
try:
    TRAX_IMAGE_MEMORY_GRAY16 = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 69
try:
    TRAX_IMAGE_MEMORY_RGB = 3
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 71
try:
    TRAX_REGION_EMPTY = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 72
try:
    TRAX_REGION_SPECIAL = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 73
try:
    TRAX_REGION_RECTANGLE = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 74
try:
    TRAX_REGION_POLYGON = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 75
try:
    TRAX_REGION_MASK = 8
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 77
try:
    TRAX_REGION_ANY = ((TRAX_REGION_RECTANGLE | TRAX_REGION_POLYGON) | TRAX_REGION_MASK)
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 79
try:
    TRAX_FLAG_VALID = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 80
try:
    TRAX_FLAG_SERVER = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 81
try:
    TRAX_FLAG_TERMINATED = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 83
try:
    TRAX_PARAMETER_VERSION = 0
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 84
try:
    TRAX_PARAMETER_CLIENT = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 85
try:
    TRAX_PARAMETER_SOCKET = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 86
try:
    TRAX_PARAMETER_REGION = 3
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 87
try:
    TRAX_PARAMETER_IMAGE = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 88
try:
    TRAX_PARAMETER_MULTIOBJECT = 5
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 90
try:
    TRAX_CHANNELS = 3
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 91
try:
    TRAX_CHANNEL_COLOR = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 92
try:
    TRAX_CHANNEL_DEPTH = 2
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 93
try:
    TRAX_CHANNEL_IR = 4
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 95
def TRAX_CHANNEL_INDEX(I):
    return (I == TRAX_CHANNEL_COLOR) and 0 or (I == TRAX_CHANNEL_DEPTH) and 1 or (I == TRAX_CHANNEL_IR) and 2 or (-1)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 100
def TRAX_CHANNEL_ID(I):
    return (I == 0) and TRAX_CHANNEL_COLOR or (I == 1) and TRAX_CHANNEL_DEPTH or (I == 2) and TRAX_CHANNEL_IR or (-1)

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 105
try:
    TRAX_LOCALHOST = '127.0.0.1'
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 108
try:
    TRAX_METADATA_MULTI_OBJECT = 1
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 116
try:
    trax_server_wait = trax_server_wait_mot
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 117
try:
    trax_server_reply = trax_server_reply_mot
except:
    pass

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 118
def trax_server_setup(M, L):
    return (trax_server_setup_v (M, L, 0))

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 119
def trax_server_setup_file(M, I, O, L):
    return (trax_server_setup_file_v (M, I, O, L, 0))

# /home/lukacu/Checkouts/vot/trax/include/trax.h: 122
def TRAX_SUPPORTS(F, M):
    return ((F & M) != 0)

trax_image = struct_trax_image# /home/lukacu/Checkouts/vot/trax/include/trax.h: 137

trax_properties = struct_trax_properties# /home/lukacu/Checkouts/vot/trax/include/trax.h: 149

trax_object_list = struct_trax_object_list# /home/lukacu/Checkouts/vot/trax/include/trax.h: 159

trax_bounds = struct_trax_bounds# /home/lukacu/Checkouts/vot/trax/include/trax.h: 168

trax_logging = struct_trax_logging# /home/lukacu/Checkouts/vot/trax/include/trax.h: 181

trax_metadata = struct_trax_metadata# /home/lukacu/Checkouts/vot/trax/include/trax.h: 195

trax_handle = struct_trax_handle# /home/lukacu/Checkouts/vot/trax/include/trax.h: 210

trax_image_list = struct_trax_image_list# /home/lukacu/Checkouts/vot/trax/include/trax.h: 217

# No inserted files

# No prefix-stripping

