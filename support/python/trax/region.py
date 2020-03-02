"""
Region description classes.
"""

import sys
from abc import abstractmethod
from ctypes import memmove, byref, c_int, c_float, cast, c_void_p
from functools import reduce
import numpy as np
import six

from .internal import trax_region_bounds, trax_region_create_polygon, \
    trax_region_create_rectangle, trax_region_get_polygon_count, trax_region_get_polygon_point, \
    trax_region_get_special, trax_region_get_type, trax_region_get_rectangle, \
    trax_region_set_polygon_point, trax_region_create_special, trax_region_create_mask, \
    trax_region_get_mask_header, trax_region_get_mask_row, trax_region_write_mask_row
from trax import RegionWrapper

class Region(object):

    SPECIAL = "special"
    RECTANGLE = "rectangle"
    POLYGON = "polygon"
    MASK = "mask"

    def __init__(self, internal):
        self._ref = RegionWrapper(internal)

    @property
    def reference(self):
        return self._ref.reference

    @staticmethod
    def wrap(internal):
        type = trax_region_get_type(internal)
        if type == 1:
            return Special(internal)
        elif type == 2:
            return Rectangle(internal)
        elif type == 4:
            return Polygon(internal)
        elif type == 8:
            return Mask(internal)

    @abstractmethod
    def type(self):
        pass

    @staticmethod
    def decode_list(intcode):
        decoded = []
        if intcode & 1:
            decoded.append(Region.SPECIAL)
        if intcode & 2:
            decoded.append(Region.RECTANGLE)
        if intcode & 4:
            decoded.append(Region.POLYGON)
        if intcode & 8:
            decoded.append(Region.MASK)
        return decoded

    @staticmethod
    def encode(strcode):
        if strcode == Region.SPECIAL:
            return 1
        elif strcode == Region.RECTANGLE:
            return 2
        elif strcode == Region.POLYGON:
            return 4
        elif strcode == Region.MASK:
            return 8

        raise IndexError("Illegal region format name {}".format(strcode))

    @staticmethod
    def encode_list(list):

        encoded = 0

        for format in list:
            encoded = encoded | Region.encode(format)

        return encoded

class Special(Region):
    """
    Special region

    :var code: Code value
    """

    @staticmethod
    def create(code):
        return Special(cast(trax_region_create_special(code), c_void_p))

    def __str__(self):
        return 'Special region (code {})'.format(trax_region_get_special(self.reference))

    @property
    def type(self):
        return Region.SPECIAL

    @property
    def code(self):
        return trax_region_get_special(self.reference)

class Rectangle(Region):
    """
    Rectangle region

    :var x: top left x coord of the rectangle region
    :var float y: top left y coord of the rectangle region
    :var float w: width of the rectangle region
    :var float h: height of the rectangle region
    """

    @staticmethod
    def create(x=0, y=0, width=0, height=0):
        """ Constructor

            :param float x: top left x coord of the rectangle region
            :param float y: top left y coord of the rectangle region
            :param float w: width of the rectangle region
            :param float h: height of the rectangle region
        """
        return Rectangle(cast(trax_region_create_rectangle(x, y, width, height), c_void_p))

    def __str__(self):
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self.reference, byref(x), byref(y), byref(width), byref(height))
        return 'Rectangle {},{} {}x{}'.format(x.value, y.value, width.value, height.value)

    @property
    def type(self):
        return Region.RECTANGLE

    def bounds(self):
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self.reference, byref(x), byref(y), byref(width), byref(height))
        return x.value, y.value, width.value, height.value

if (sys.version_info > (3, 0)):
    class PolygonIterator(object):

        def __init__(self, polygon):
            self.polygon = polygon
            self.position = 0
            self.size = polygon.size()

        def __next__(self):
            if self.position == self.size:
                raise StopIteration
            result = self.polygon.get(self.position)
            self.position = self.position + 1
            return result
else:
    class PolygonIterator(object):

        def __init__(self, polygon):
            self.polygon = polygon
            self.position = 0
            self.size = polygon.size()

        def next(self):
            if self.position == self.size:
                raise StopIteration
            result = self.polygon.get(self.position)
            self.position = self.position + 1
            return result

class Polygon(Region):
    """
    Polygon region

    :var list points: List of points as tuples [(x1,y1), (x2,y2),...,(xN,yN)]
    :var int count: number of points
    """
    @staticmethod
    def create(points):
        """
        Constructor

        :param list points: List of points as tuples [(x1,y1), (x2,y2),...,(xN,yN)]
        """
        assert(isinstance(points, list))
        assert(len(points) > 2)
        assert(reduce(lambda x, y: x and y, [isinstance(p, tuple) for p in points]))

        poly = cast(trax_region_create_polygon(len(points)), c_void_p)

        for i in six.moves.range(0, len(points)):
            trax_region_set_polygon_point(poly, i, points[i][0], points[i][1])

        return Polygon(poly)

    def __str__(self):
        return 'Polygon with {} points'.format(self.size())

    @property
    def type(self):
        return Region.POLYGON

    def size(self):
        return trax_region_get_polygon_count(self.reference)

    def __getitem__(self, i):
        return self.get(i)

    def get(self, i):
        if i < 0 or i >= self.size():
            raise IndexError("Index {} is invalid".format(i))
        x = c_float()
        y = c_float()
        trax_region_get_polygon_point(self.reference, i, byref(x), byref(y))
        return x.value, y.value

    def __iter__(self):
        return PolygonIterator(self)


class Mask(Region):
    """Mask region

    Raises:
        IndexError: [description]

    Returns:
        [type] -- [description]
    """
    @staticmethod
    def create(source, x = 0, y = 0):
        """Creates a new mask region object

        Arguments:
            source {np.ndarray} -- source mask as NumPy array

        Keyword Arguments:
            x {int} -- horizontal offset (default: {0})
            y {int} -- vertical offset (default: {0})

        Returns:
            Mask -- reference to a new mask region
        """
        assert(len(source.shape) == 2 and source.dtype == np.uint8)

        mask = cast(trax_region_create_mask(x, y, source.shape[1], source.shape[0]), c_void_p)

        data = trax_region_write_mask_row(mask, 0)
        memmove(data, source.ctypes.data, source.nbytes)

        return Mask(mask)

    def __str__(self):
        width = c_int()
        height = c_int()
        x = c_int()
        y = c_int()
        trax_region_get_mask_header(self.reference, byref(x), byref(y), byref(width), byref(height))
        return 'Mask of size {}x{} with offset x={}, y={}'.format(width.value, height.value, x.value, y.value)

    @property
    def type(self):
        return Region.MASK

    def size(self):
        width = c_int()
        height = c_int()
        trax_region_get_mask_header(self.reference, None, None, byref(width), byref(height))
        return width.value, height.value

    def offset(self):
        x = c_int()
        y = c_int()
        trax_region_get_mask_header(self.reference, byref(x), byref(y), None, None)
        return x.value, y.value

    def get(self, i):
        if i < 0 or i >= self.size():
            raise IndexError("Index {} is invalid".format(i))
        x = c_float()
        y = c_float()
        trax_region_get_polygon_point(self.reference, i, byref(x), byref(y))
        return x.value, y.value

    def array(self, normalize=False):
        width = c_int()
        height = c_int()
        x = c_int()
        y = c_int()
        trax_region_get_mask_header(self.reference, byref(x), byref(y), byref(width), byref(height))
        mat = np.empty((height.value, width.value), dtype=np.uint8)

        data = trax_region_get_mask_row(self.reference, 0)
        memmove(mat.ctypes.data, data, mat.nbytes)

        if not normalize:
            return mat

        return np.pad(mat, pad_width=[(y.value, 0), (x.value, 0)], mode='constant')
