import sys
import traceback
import weakref
from .internal import trax_image_release, trax_region_release, trax_cleanup, trax_properties_release

from ctypes import byref, cast, c_void_p

class OwnerRef(weakref.ref):
    pass

def _run_finalizer(ref):
    """Internal weakref callback to run finalizers"""
    del _finalize_refs[id(ref)]
    finalizer = ref.finalizer
    item = ref.item
    try:
        finalizer(item)
    except Exception:
        print("Exception {}:".format(finalizer))
        traceback.print_exc()

_finalize_refs = {}

def _track_for_finalization(owner, item, finalizer):
    """Register an object for finalization.

    ``owner`` is the the object which is responsible for ``item``.
    ``finalizer`` will be called with ``item`` as its only argument when
    ``owner`` is destroyed by the garbage collector.
    """
    ref = OwnerRef(owner, _run_finalizer)
    ref.item = item
    ref.finalizer = finalizer
    _finalize_refs[id(ref)] = ref
    return ref

class Wrapper(object):

	def __init__(self, reference, finalizer):
		self.ref = _track_for_finalization(self, reference, finalizer)

	def reference(self):
		return self.ref.item


class ImageWrapper(Wrapper):

	def __init__(self, reference):
		super(ImageWrapper, self).__init__(reference, lambda x: trax_image_release(byref(x)))

class RegionWrapper(Wrapper):

	def __init__(self, reference):
		super(RegionWrapper, self).__init__(reference, lambda x: trax_region_release(byref(cast(x, c_void_p))))

class PropertiesWrapper(Wrapper):

	def __init__(self, reference):
		super(PropertiesWrapper, self).__init__(reference, lambda x: trax_properties_release(byref(x)))

class HandleWrapper(Wrapper):

	def __init__(self, reference):
		super(HandleWrapper, self).__init__(reference, lambda x: trax_cleanup(byref(x)))