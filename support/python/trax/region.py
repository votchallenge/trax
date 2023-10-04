"""
Region description classes. These are used to describe the object location in the image.
"""

__all__ = ['Region', 'Special', 'Rectangle', 'Polygon', 'Mask']

import sys
from abc import abstractmethod
from ctypes import memmove, byref, c_int, c_float, cast, c_void_p
from functools import reduce
import numpy as np
import six

from ._ctypes import trax_region_create_polygon, \
    trax_region_create_rectangle, trax_region_get_polygon_count, trax_region_get_polygon_point, \
    trax_region_get_special, trax_region_get_type, trax_region_get_rectangle, \
    trax_region_set_polygon_point, trax_region_create_special, trax_region_create_mask, \
    trax_region_get_mask_header, trax_region_get_mask_row, trax_region_write_mask_row

class Region(object):
    """Base class for region descriptions."""

    SPECIAL = "special"
    RECTANGLE = "rectangle"
    POLYGON = "polygon"
    MASK = "mask"

    def __init__(self, internal):
        """Constructor
        
        Args:
            internal (c_void_p): Internal region pointer.
        """
        from . import RegionWrapper
        self._ref = RegionWrapper(internal)

    @property
    def reference(self):
        """Internal region pointer."""
        return self._ref.reference

    @staticmethod
    def wrap(internal):
        """Wrap internal region pointer into a region object."""
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
    def type(self) -> str:
        """Region type."""
        pass

    @staticmethod
    def decode_list(intcode):
        """Decode region format from integer code.
        
        Args:
            intcode (int): Integer code.
        
        Returns:
            [list]: List of region format names.
        """
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
        """Encode region format to integer code. 
        
        Args:
            strcode (str): Region format name.
        """
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
        """Encode region format list to integer code.
        
        Args:
            list (list): List of region format names.
        """

        encoded = 0

        for format in list:
            encoded = encoded | Region.encode(format)

        return encoded

class Special(Region):
    """
    Special region
    """

    @staticmethod
    def create(code: int):
        """Constructor

        Args:
            code (int): Special region code.
        """
        return Special(cast(trax_region_create_special(code), c_void_p))

    def __str__(self):
        """String representation of the region.
        
        Returns:
            str: String representation of the region.
        """
        return 'Special region (code {})'.format(trax_region_get_special(self.reference))

    @property
    def type(self) -> str:
        """Region type.
        
        Returns:
            str: Region type."""
        return Region.SPECIAL

    @property
    def code(self):
        return trax_region_get_special(self.reference)

class Rectangle(Region):
    """
    Rectangle region description
    """

    @staticmethod
    def create(x=0, y=0, width=0, height=0):
        """ Constructor

        Args:
            x (float): X coordinate of the top left corner.
            y (float): Y coordinate of the top left corner.
            width (float): Width of the rectangle.
            height (float): Height of the rectangle.

        Returns:
            Rectangle: Rectangle region description.
        """
        return Rectangle(cast(trax_region_create_rectangle(x, y, width, height), c_void_p))

    def __str__(self):
        """String representation of the region.
        
        Returns:
            str: String representation of the region.
        """
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self.reference, byref(x), byref(y), byref(width), byref(height))
        return 'Rectangle {},{} {}x{}'.format(x.value, y.value, width.value, height.value)

    @property
    def type(self) -> str:
        """Region type.
        
        Returns:
            str: Region type.
        """
        return Region.RECTANGLE

    def bounds(self):
        """Bounding box of the region.
        
        Returns:
            tuple: Bounding box of the region.
        """
        x = c_float()
        y = c_float()
        width = c_float()
        height = c_float()
        trax_region_get_rectangle(self.reference, byref(x), byref(y), byref(width), byref(height))
        return x.value, y.value, width.value, height.value

if (sys.version_info > (3, 0)):
    class PolygonIterator(object):
        """Polygon iterator for Python 3.x"""

        def __init__(self, polygon):
            """Constructor 
            
            Args:
                polygon (Polygon): Polygon region.
            """
            self.polygon = polygon
            self.position = 0
            self.size = polygon.size()

        def __next__(self):
            """Next element in the polygon.
            
            Returns:
                tuple: Next point in the polygon.
            """
            if self.position == self.size:
                raise StopIteration
            result = self.polygon.get(self.position)
            self.position = self.position + 1
            return result
else:
    class PolygonIterator(object):
        """Polygon iterator for Python 2.x"""

        def __init__(self, polygon):
            """Constructor"""
            self.polygon = polygon
            self.position = 0
            self.size = polygon.size()

        def next(self):
            """Next element in the polygon."""
            if self.position == self.size:
                raise StopIteration
            result = self.polygon.get(self.position)
            self.position = self.position + 1
            return result

class Polygon(Region):
    """
    Polygon region
    """
    @staticmethod
    def create(points):
        """
        Constructor

        Args:
            points: list of points in the polygon
        """
        assert(isinstance(points, list))
        assert(len(points) > 2)
        assert(reduce(lambda x, y: x and y, [isinstance(p, tuple) for p in points]))

        poly = cast(trax_region_create_polygon(len(points)), c_void_p)

        for i in six.moves.range(0, len(points)):
            trax_region_set_polygon_point(poly, i, points[i][0], points[i][1])

        return Polygon(poly)

    def __str__(self):
        """String representation of the polygon region
        
        Returns:
            string: string representation of the polygon region
        """
        return 'Polygon with {} points'.format(self.size())

    @property
    def type(self):
        """Type of the region
        
        Returns:
            int: type of the region
        """
        return Region.POLYGON

    def size(self):
        """Number of points in the polygon
        
        Returns:
            int: number of points in the polygon
        """
        return trax_region_get_polygon_count(self.reference)

    def __getitem__(self, i: int) -> tuple:
        """Get point at index i
        
        Args:
            i (int): index of the point
        
        Returns:
            tuple: point at index i
        """
        return self.get(i)

    def get(self, i: int) -> tuple:
        """Get point at index i

        Args:
            i (int): index of the point

        Returns:
            tuple: point at index i
        """

        if i < 0 or i >= self.size():
            raise IndexError("Index {} is invalid".format(i))
        x = c_float()
        y = c_float()
        trax_region_get_polygon_point(self.reference, i, byref(x), byref(y))
        return x.value, y.value

    def __iter__(self):
        """Iterator over the points in the polygon
        
        Returns:
            PolygonIterator: iterator over the points in the polygon
        """
        return PolygonIterator(self)

class Mask(Region):
    """Mask region wrapper.
    """
    @staticmethod
    def create(source, x = 0, y = 0):
        """Creates a new mask region object

        Args:
            source (numpy.ndarray): source image
            x (int, optional): horizontal offset. Defaults to 0.
            y (int, optional): vertical offset. Defaults to 0.

        Returns:
            Mask: mask region object
        """
        assert(len(source.shape) == 2 and source.dtype == np.uint8)

        mask = cast(trax_region_create_mask(x, y, source.shape[1], source.shape[0]), c_void_p)

        data = trax_region_write_mask_row(mask, 0)
        memmove(data, source.ctypes.data, source.nbytes)

        return Mask(mask)

    def __str__(self):
        """Returns a string representation of the mask region object
        
        Returns:
            str: string representation of the mask region object
        """
        width = c_int()
        height = c_int()
        x = c_int()
        y = c_int()
        trax_region_get_mask_header(self.reference, byref(x), byref(y), byref(width), byref(height))
        return 'Mask of size {}x{} with offset x={}, y={}'.format(width.value, height.value, x.value, y.value)

    @property
    def type(self):
        """Returns the type of the region object
        
        Returns:
            int: type of the region object"""
        return Region.MASK

    def size(self):
        """Returns the size of the mask region object

        Returns:
            tuple: size of the mask region object
        """
        width = c_int()
        height = c_int()
        trax_region_get_mask_header(self.reference, None, None, byref(width), byref(height))
        return width.value, height.value

    def offset(self):
        """Returns the offset of the mask region object
        
        Returns:
            tuple: offset of the mask region object
        """
        x = c_int()
        y = c_int()
        trax_region_get_mask_header(self.reference, byref(x), byref(y), None, None)
        return x.value, y.value

    def get(self, i):
        """Returns the i-th point of the mask region object
        
        Args:
            i (int): index of the point
        """
        if i < 0 or i >= self.size():
            raise IndexError("Index {} is invalid".format(i))
        x = c_float()
        y = c_float()
        trax_region_get_polygon_point(self.reference, i, byref(x), byref(y))
        return x.value, y.value

    def array(self, normalize=False):
        """Returns the mask region object as a numpy array. If normalize is True, the mask is padded with zeros for the offset.
        
        Args:
            normalize (bool, optional): whether to normalize the mask by adding the offset. Defaults to False.
            
        Returns:
            numpy.ndarray: mask region object as a numpy array
        """
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
