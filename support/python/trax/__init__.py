
""" Python bindings for the TraX protocol, main module. """

__all__ = \
    ['TraxException', 'FileImage', 'URLImage', 'Server',
    'MemoryImage', 'BufferImage', 'Region', 'Image', 'Polygon',
    'Rectangle', 'Special', 'Mask', 'TraxStatus', 'Properties']

import os
import traceback
import weakref
from ctypes import py_object, c_void_p, cast, byref, POINTER

from ._ctypes import \
    trax_properties_create, \
    trax_properties_get, trax_properties_set, trax_object_list, \
    trax_image_release, trax_region_release, trax_object_list_release, \
    trax_cleanup, trax_properties_release, trax_image_list_release, \
    struct_trax_handle, struct_trax_image, struct_trax_image_list, \
    struct_trax_properties, struct_trax_object_list, POINTER

trax_image_p = POINTER(struct_trax_image)
trax_image_list_p = POINTER(struct_trax_image_list)
trax_region_p = c_void_p
trax_object_list_p = POINTER(trax_object_list)
trax_properties_p = POINTER(struct_trax_properties)

class TraxException(Exception):
    """ Base class for all TraX related exceptions """
    pass

class TraxStatus(object):
    """ TraX status codes wrapper class """
    ERROR = "error"
    OK = "ok"
    HELLO = "hello"
    INITIALIZE = "initialize"
    FRAME = "frame"
    QUIT = "quit"
    STATE = "state"

    @staticmethod
    def decode(intcode):
        """ Decode integer status code to string
        
        Args:
            intcode (int): integer status code
        
        Returns:
            str: string status code
        """
        if intcode == -1:
            return TraxStatus.ERROR
        elif intcode == 0:
            return TraxStatus.OK
        elif intcode == 1:
            return TraxStatus.HELLO
        elif intcode == 2:
            return TraxStatus.INITIALIZE
        elif intcode == 3:
            return TraxStatus.FRAME
        elif intcode == 4:
            return TraxStatus.QUIT
        elif intcode == 5:
            return TraxStatus.STATE

class Logger(object):
    """ Base class for all TraX loggers, subclasses should implement handle() method. """

    def __init__(self):
        """ Initialize logger"""
        self._interrupted = False

    def __call__(self, buffer, length, _):
        """ Call logger with buffer and length, this method is called from C code, handles buffer decoding and calls handle() method. It also handles KeyboardInterrupt exception.
        
        Args:
            buffer (bytes): buffer to decode
            length (int): length of the buffer
            _ (ctypes.c_void_p): unused
            
        """
        try:
            if not buffer:
                return
            message = buffer[:length].decode("utf-8")
            self.handle(message)
        except KeyboardInterrupt:
            self._interrupted = True

    def handle(self, message: str):
        """ Handle message, should be implemented in subclasses. 
        
        Args:
            message (str): message to handle
        """
        pass

    @property
    def interrupted(self):
        """ Returns True if logger was interrupted by KeyboardInterrupt, False otherwise."""
        return self._interrupted

class ConsoleLogger(Logger):
    """ Logger that prints all messages to standard output."""

    def handle(self, message):
        """ Handle message by printing it to standard output.
        
        Args:
            message (str): message to handle
        """
        print(message, end='')

class FileLogger(Logger):
    """ Logger that writes all messages to a file."""

    def __init__(self, filename: str):
        """ Initialize logger with filename.
        
        Args:
            filename (str): name of the file to write messages to
        """
        super().__init__()
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        self._fp = open(filename, "w")

    def handle(self, message: str):
        """ Handle message by writing it to a file.
        
        Args:
            message (str): message to handle
        """
        self._fp.write(message)

    def __del__(self):
        """ Close file when logger object is destroyed."""
        self._fp.close()

class ProxyLogger(Logger):
    """ Logger that proxies all messages to a callback function. """

    def __init__(self, callback: callable):
        """ Initialize logger with callback function.
        
        Args:
            callback (callable): callback function to call with message
        """
        super().__init__()
        self._callback = callback

    def handle(self, message):
        """ Handle message by calling callback function.
        
        Args:
            message (str): message to handle
        """
        self._callback(message)

def _run_finalizer(ref):
    """Internal weakref callback to run finalizers"""
    del _finalize_refs[id(ref)]
    finalizer = ref.finalizer
    item = ref.item
    if finalizer:
        try:
            finalizer(item)
        except Exception:
            print("Exception {}:".format(finalizer))
            traceback.print_exc()

class OwnerRef(weakref.ref):
    """Weak reference that calls a finalizer when the object goes away."""

    __slots__ = "item", "finalizer"

    def __new__(type, ob, item, finalizer):
        """Create a new weak reference that tracks the owner object."""
        return weakref.ref.__new__(type, ob, _run_finalizer)

    def __init__(self, owner, item, callback):
        """Initialize a weak reference with a finalizer."""
        super(OwnerRef, self).__init__(owner)
        self.item = item
        self.finalizer = callback

_finalize_refs = {}

def _track_for_finalization(owner, item, finalizer):
    """Register an object for finalization.

    Args:
        owner: The object that owns the object being finalized.
        item: The object to finalize.
        finalizer: The finalizer function to call when the object is finalized.
    """
    ref = OwnerRef(owner, item, finalizer)
    _finalize_refs[id(ref)] = ref
    return ref

class Wrapper(object):
    """ Base class for all TraX wrappers, subclasses should implement reference property."""

    def __init__(self, ctype, reference, finalizer):
        if not reference is None:
            if not isinstance(reference, ctype):
                raise RuntimeError("Not a ctype {}".format(type(ctype)))
            self.ref = _track_for_finalization(self, reference, finalizer)
        else:
            self.ref = None

    @property
    def reference(self):
        """ Returns reference to the wrapped structure.
        
        Returns:
            c_void_p: reference to the wrapped structure
        """
        if not self.ref:
            return None
        return self.ref.item

    def __nonzero__(self):
        """ Returns True if reference is not None, False otherwise."""
        return not self.ref is None

def _debug_wrapper(cb, message):
    """ Debug wrapper for various callbacks."""
    def wrapper(*args, **kwargs):
        print(message, args, kwargs)
        cb(*args, **kwargs)
    return wrapper

class ImageWrapper(Wrapper):
    """ Wrapper for trax_image_t structure."""

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_image_t structure.
        
        Args:
            reference (c_void_p): reference to trax_image_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(POINTER(struct_trax_image), reference, lambda x: trax_image_release(byref(x)) if owner else None)

class ImageListWrapper(Wrapper):
    """ Wrapper for trax_image_list_t structure."""

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_image_list_t structure.
        
        Args:
            reference (c_void_p): reference to trax_image_list_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(POINTER(struct_trax_image_list), reference, lambda x: trax_image_list_release(byref(x)) if owner else None)

class ObjectListWrapper(Wrapper):
    """ Wrapper for trax_object_list_t structure.
    """

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_object_list_t structure.
        
        Args:
            reference (c_void_p): reference to trax_object_list_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(POINTER(struct_trax_object_list), reference, lambda x: trax_object_list_release(byref(x)) if owner else None)

class RegionWrapper(Wrapper):
    """ Wrapper for trax_region_t structure.
    """

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_region_t structure.
        
        Args:
            reference (c_void_p): reference to trax_region_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(c_void_p, reference, lambda x: trax_region_release(byref(cast(x, c_void_p))) if owner else None)

class PropertiesWrapper(Wrapper):
    """ Wrapper for trax_properties_t structure."""

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_properties_t structure.
        
        Args:
            reference (c_void_p): reference to trax_properties_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(POINTER(struct_trax_properties), reference, lambda x: trax_properties_release(byref(x)) if owner else None)

class HandleWrapper(Wrapper):
    """ Wrapper for trax_handle_t structure."""

    def __init__(self, reference, owner=True):
        """ Initialize wrapper with reference to trax_handle_t structure.
        
        Args:
            reference (c_void_p): reference to trax_handle_t structure
            owner (bool): whether to take ownership of the structure
        """
        super().__init__(POINTER(struct_trax_handle), reference, lambda x: trax_cleanup(byref(x)) if owner else None)

def wrap_image_list(list):
    """ Wrap image list internal C structure into a dictionary of images.
    
    Args:
        list (c_void_p): image list to wrap
    """
    from ._ctypes import trax_image_list_get

    channels = [ImageChannel.COLOR, ImageChannel.DEPTH, ImageChannel.IR]
    wrapped = {}

    for channel in channels:
        img = trax_image_list_get(list, ImageChannel.encode(channel))
        if not img:
            continue
        wrapped[channel] = Image.wrap(img)

    return wrapped

def wrap_object_list(list):
    """ Wrap object list internal C structure into a list of regions.
    
    Args:
        list (c_void_p): object list to wrap
    """
    from ._ctypes import trax_object_list_count, \
        trax_object_list_get, trax_object_list_properties, \
        trax_region_clone
    import ctypes

    objects = []
    for i in range(trax_object_list_count(list)):
        region = trax_object_list_get(list, i)
        properties = Properties(trax_object_list_properties(list, i), False).dict()
        r = trax_region_clone(region)
        objects.append((Region.wrap(ctypes.cast(r, c_void_p)), properties))
    return objects


def wrap_images(images):
    """ Wrap dictionary of images into image list internal C structure.
    
    Args:
        images (dict): dictionary of images
    """
    from ._ctypes import trax_image_list_create, trax_image_list_set

    channels = [ImageChannel.COLOR, ImageChannel.DEPTH, ImageChannel.IR]
    tlist = ImageListWrapper(trax_image_list_create())

    for channel in channels:
        if not channel in images:
            continue

        trax_image_list_set(tlist.reference, images[channel].reference, ImageChannel.encode(channel))

    return tlist

def wrap_objects(objects):
    """ Wrap list of regions into object list internal C structure.
    
    Args:
        objects (list): list of regions
    """
    from ._ctypes import trax_properties_append, trax_object_list_set, \
         trax_object_list_properties, trax_object_list_create
    tlist = ObjectListWrapper(trax_object_list_create(len(objects)))

    for i, (region, properties) in enumerate(objects):
        properties =  Properties(properties, False)
        trax_object_list_set(tlist.reference, i, region.reference)
        trax_properties_append(properties.reference, trax_object_list_properties(tlist.reference, i), 0)

    return tlist

class Properties(object):
    """ Wrapper for properties structure. Supports a dictionary-like interface."""

    def __init__(self, data=None, owner=True):
        """ Create a new properties structure.
        
        Args:
            data (dict): dictionary of properties or a properties C structure reference to wrap
            owner (bool): if True, the structure will be released when the wrapper is destroyed    
        """
        if not data:
            self._ref = PropertiesWrapper(trax_properties_create())
        elif isinstance(data, trax_properties_p):
            self._ref = PropertiesWrapper(data, owner)
        elif isinstance(data, dict):
            self._ref = PropertiesWrapper(trax_properties_create())
            for key, value in data.items():
                trax_properties_set(self._ref.reference, key.encode('utf8'), str(value).encode('utf8'))

    def copy(self, source: "Properties"):
        """ Copy properties from another properties structure.
        
        Args:
            source (Properties): source properties structure
        """
        for k, v in source.dict().items():
            self[k] = v

    @property
    def reference(self):
        """ Get a reference to the internal C structure.
        
        Returns:
            c_void_p: reference of the internal C structure
        """
        return self._ref.reference

    def __getitem__(self, key: str):
        """ Get a property value.
        
        Args:
            key (str): property key
        """
        return trax_properties_get(self._ref.reference, key)

    def __setitem__(self, key: str, value: str):
        """ Set a property value.
        
        Args:
            key (str): property key
            value (str): property value
        """
        trax_properties_set(self._ref.reference, key.encode('utf8'), str(value).encode('utf8'))

    def get(self, key: str, default: str = None):
        """ Get a property value.
        
        Args:
            key (str): property key
            default (str): default value if the property is not found
        
        Returns:
            str: property value
        """
        value = trax_properties_get(self._ref.reference, key.encode('utf8'))
        if value == None:
            return default
        return value.decode('utf8') if not value is None else None

    def set(self, key: str, value: str):
        """ Set a property value.
        
        Args:
            key (str): property key
            value (str): property value
        """
        trax_properties_set(self._ref.reference, key.encode('utf8'), str(value).encode('utf8'))

    def dict(self):
        """ Return a dictionary of properties. The dictionary is a copy of the internal structure entries."""
        from ._ctypes import trax_properties_enumerate, trax_enumerator

        result = dict()
        fun = lambda key, value, obj: cast(obj, py_object).value.setdefault(key.decode("utf8"), value.decode("utf8"))
        trax_properties_enumerate(self._ref.reference, trax_enumerator(fun), py_object(result))
        return result

from .server import Server
from .region import Polygon, Rectangle, Special, Mask, Region
from .image import Image, MemoryImage, BufferImage, FileImage, URLImage, ImageChannel
