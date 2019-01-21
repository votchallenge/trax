"""
Region description classes.
"""

import sys
import traceback
import weakref
from abc import abstractmethod
from ctypes import memmove, byref, c_int, c_float

from .internal import *
from .wrapper import RegionWrapper
from trax import TraxException

if (sys.version_info > (3, 0)):
    xrange = range

class Region(object):

    SPECIAL = "special"
    RECTANGLE = "rectangle"
    POLYGON = "polygon"

    def __init__(self, internal):
        self._ref = RegionWrapper(internal)

    @staticmethod
    def wrap(internal):
        type = trax_region_get_type(internal)
        if type == 1:
            return Special(internal)
        elif type == 2:
            return Rectangle(internal)     
        elif type == 4:
            return Polygon(internal) 

    @abstractmethod
    def type(self):
        pass

    @staticmethod
    def encode(strcode):
        if strcode == Region.SPECIAL:
            return 1
        elif strcode == Region.RECTANGLE:
            return 2      
        elif strcode == Region.POLYGON:
            return 4

        raise IndexError("Illegal region format name {}".format(strcode))

    @staticmethod
    def encode_list(list):

        encoded = 0

        for format in list:
            encoded = encoded | Region.encode(format)

        return encoded

def encode_region_formats(list):

    encoded = 0

    for format in list:
        encoded = encoded | _region_format_map_code[format]

    return encoded

class Special(Region):
    """
    Special region

    :var code: Code value
    """

    @staticmethod
    def create(code):
        return Special(trax_region_create_special(code))

    def __str__(self):
        return 'Special region (code {})'.format(trax_region_get_special(self._ref.reference()))

    def type(self):
        return SPECIAL

    def code(self):
        return trax_region_get_special(self._ref.reference())

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
        return Rectangle(trax_region_create_rectangle(x, y, width, height))

    def __str__(self):
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self._ref.reference(), byref(x), byref(y), byref(width), byref(height))
        return 'Rectangle {},{} {}x{}'.format(x.value, y.value, width.value, height.value)

    def type(self):
        return RECTANGLE

    def bounds(self):
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self._ref.reference(), byref(x), byref(y), byref(width), byref(height))
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
        assert(reduce(lambda x,y: x and y, [isinstance(p, tuple) for p in points]))

        poly = trax_region_create_polygon(len(points))

        for i in xrange(0, len(points)):
            trax_region_set_polygon_point(poly, i, points[i][0], points[i][1])

        return Polygon(poly)

    def __str__(self):
        return 'Polygon with {} points'.format(self.size())

    def type(self):
        return POLYGON

    def size(self):
        return trax_region_get_polygon_count(self._ref.reference())

    def __getitem__(self, i):
        return self.get(i)

    def get(self, i):
        if i < 0 or i >= self.size():
            raise IndexError("Index {} is invalid".format(i))
        x = c_float()
        y = c_float()
        trax_region_get_polygon_point(self._ref.reference(), i, byref(x), byref(y))
        return x.value, y.value

    def __iter__(self):
        return PolygonIterator(self)