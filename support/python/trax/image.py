"""
Image description classes.
"""

#from __future__ import absolute

__all__ = ['Image', 'FileImage', 'URLImage', 'MemoryImage', 'BufferImage', 'ImageChannel']

from abc import abstractmethod
from ctypes import memmove, byref, c_int, string_at

from ._ctypes import \
        trax_image_create_path, trax_image_create_memory, \
        trax_image_get_memory_header, trax_image_get_type, \
        trax_image_get_path, trax_image_get_url, \
        trax_image_create_url, trax_image_get_memory_row, \
        trax_image_write_memory_row, trax_image_get_buffer, \
        trax_image_create_buffer

class ImageChannel(object):
    """ Image channel identifier. """

    COLOR = "color"
    DEPTH = "depth"
    IR = "ir"

    @staticmethod
    def decode(intcode: int):
        """ Decode an image channel from an integer code.
        
        Args:
            intcode (int): Integer code.
        """
        if intcode == 1:
            return ImageChannel.COLOR
        elif intcode == 2:
            return ImageChannel.DEPTH
        elif intcode == 4:
            return ImageChannel.IR
        raise IndexError("Illegal image channel identifier {}".format(intcode))

    @staticmethod
    def decode_list(intcode: int):
        """ Decode a list of image channels from an integer code.
        
        Args:
            intcode (int): Integer code.
        """
        decoded = []
        if intcode & 1:
            decoded.append(ImageChannel.COLOR)
        if intcode & 2:
            decoded.append(ImageChannel.DEPTH)
        if intcode & 4:
            decoded.append(ImageChannel.IR)
        return decoded

    @staticmethod
    def encode(strcode):
        """ Encode an image channel into an integer code."""
        if strcode == ImageChannel.COLOR:
            return 1
        elif strcode == ImageChannel.DEPTH:
            return 2
        elif strcode == ImageChannel.IR:
            return 4
        raise IndexError("Illegal image channel name {}".format(strcode))

    @staticmethod
    def encode_list(list):
        """ Encode a list of image channels into an integer code.
        
        Args:
            list (list): List of image channels.
        
        Returns:
            int: Integer code.
        """
        encoded = 0

        for format in list:
            encoded = encoded | ImageChannel.encode(format)

        return encoded

class Image(object):
    """ Image description class. """

    PATH = "path"
    URL = "url"
    MEMORY = "memory"
    BUFFER = "buffer"

    """ Image saved in memory as a numpy array """
    def __init__(self, internal):
        from . import ImageWrapper
        self._ref = ImageWrapper(internal)

    @property
    def reference(self):
        """ Get reference to the internal object.
    
        Returns:
            c_void_p: Reference to the internal object.
        """
        return self._ref.reference

    @staticmethod
    def decode_list(intcode):
        """ Decode a list of image formats from an integer code.
        
        Args:
            intcode (int): Integer code.
        """
        decoded = []
        if intcode & 1:
            decoded.append(Image.PATH)
        if intcode & 2:
            decoded.append(Image.URL)
        if intcode & 4:
            decoded.append(Image.MEMORY)
        if intcode & 8:
            decoded.append(Image.BUFFER)
        return decoded

    @staticmethod
    def wrap(internal):
        type = trax_image_get_type(internal)
        if type == 1:
            return FileImage(internal)
        if type == 2:
            return URLImage(internal)
        if type == 4:
            return MemoryImage(internal)
        if type == 8:
            return BufferImage(internal)

    @abstractmethod
    def type(self):
        """ Get image type """
        pass

    @staticmethod
    def encode(strcode):
        if strcode == Image.PATH:
            return 1
        elif strcode == Image.URL:
            return 2
        elif strcode == Image.MEMORY:
            return 4
        elif strcode == Image.BUFFER:
            return 8
        raise IndexError("Illegal image format name {}".format(strcode))


    @staticmethod
    def encode_list(list):
        """ Encode a list of image formats into an integer code.
        
        Args:
            list (list): List of image formats.
        
        Returns:
            int: Integer code.
        """

        encoded = 0

        for format in list:
            encoded = encoded | Image.encode(format)

        return encoded

class FileImage(Image):
    """
    Image saved in a local file
    """

    @staticmethod
    def create(path = None):
        return FileImage(trax_image_create_path(path.encode('utf8')))

    def __str__(self):
        """ Get description """
        return "File resource at '{}'".format(trax_image_get_path(self.reference))

    def type(self):
        return Image.PATH

    def path(self):
        return trax_image_get_path(self.reference).decode('utf8')

class URLImage(Image):
    """Image saved in a local or remote resource
    """

    @staticmethod
    def create(url: str = None):
        """ Create a new URL image resource.
        
        Args:
            url (str): URL of the resource.
        """
        return URLImage(trax_image_create_url(url.encode('utf8')))

    def __str__(self) -> str:
        """ Get description 
        
        Returns:
            str: Description of the image
        """
        return "URL resource at '{}'".format(trax_image_get_url(self.reference))

    def type(self):
        """ Get image type
        
        Returns:
            str: Image type
        """
        return Image.URL

    def url(self):
        """ Get image URL
        
        Returns:
            str: Image URL
        """
        return trax_image_get_url(self.reference).decode('utf8')


IMAGE_MEMORY_GRAY8 = 1
IMAGE_MEMORY_GRAY16 = 2
IMAGE_MEMORY_RGB = 3

_image_memory_map_string = {IMAGE_MEMORY_RGB : "rgb", IMAGE_MEMORY_GRAY8: "gray8", IMAGE_MEMORY_GRAY16: "gray16" }
_image_memory_map_ch = {IMAGE_MEMORY_RGB : 3, IMAGE_MEMORY_GRAY8: 1, IMAGE_MEMORY_GRAY16: 1}

class MemoryImage(Image):
    """ Image saved in memory as a numpy array """

    @staticmethod
    def create(image: "numpy.ndarray"):
        """ Create a new memory image resource. 

        Args:
            image (np.ndarray): Image data.
        """

        from . import TraxException

        try:
            import numpy as np

            assert(isinstance(image, np.ndarray))

            image = np.ascontiguousarray(image)

        except ImportError:
            from . import TraxException
            raise TraxException("Numpy is not installed")

        width = image.shape[1]
        height = image.shape[0]
        format = 0
        if len(image.shape) == 3 and image.shape[2] == 3:
            if image.itemsize == 1:
                format = IMAGE_MEMORY_RGB
        elif len(image.shape) == 2 or image.shape[2] == 1:
            if image.itemsize == 1:
                format = IMAGE_MEMORY_GRAY8
            elif image.itemsize == 2:
                format = IMAGE_MEMORY_GRAY16

        if format == 0:
            raise TraxException("Image format not supported")

        timage = trax_image_create_memory(width, height, format)

        data = trax_image_write_memory_row(timage, 0)
        
        memmove(data, image.ctypes, image.nbytes)
        return MemoryImage(timage)

    def __str__(self):
        """ Get description """
        width = c_int()
        height = c_int()
        format = c_int()
        trax_image_get_memory_header(self.reference, byref(width), byref(height), byref(format))

        return "Raw image of size {}x{}, format {}".format(width.value, height.value, _image_memory_map_string[format.value])

    def type(self):
        return Image.MEMORY

    def array(self):
        try:
            import numpy as np
            _image_memory_map_dtype = {IMAGE_MEMORY_RGB : np.uint8, IMAGE_MEMORY_GRAY8: np.uint8, IMAGE_MEMORY_GRAY16: np.uint16 }
            
            width = c_int()
            height = c_int()
            format = c_int()
            trax_image_get_memory_header(self.reference, byref(width), byref(height), byref(format))
            mat = np.ones((height.value, width.value, _image_memory_map_ch[format.value]), dtype=_image_memory_map_dtype[format.value])

            data = trax_image_get_memory_row(self.reference, 0)

            memmove(mat.ctypes.data, data, mat.nbytes)

            return mat
        except ImportError:
            from . import TraxException
            raise TraxException("NumPy is not installed")


IMAGE_BUFFER_JPEG = 1
IMAGE_BUFFER_PNG = 2

_image_buffer_map_string = {IMAGE_BUFFER_JPEG : "jpeg", IMAGE_BUFFER_PNG: "png"}

class BufferImage(Image):
    """ Image encoded in a memory buffer stored in JPEG or PNG file format."""
    @staticmethod
    def create(data: bytes):
        """ Create image from a memory buffer
        
        Args:
            data: A memory buffer containing the encoded image.
        """
        tbuffer = trax_image_create_buffer(len(data), data)
        return BufferImage(tbuffer)

    def __str__(self):
        """ Get description """
        length = c_int()
        format = c_int()
        trax_image_get_buffer(self.reference, byref(length), byref(format))
        return "Encoded image (format: {}, size: {})".format(_image_buffer_map_string[format.value], length.value)

    def type(self):
        """ Get image type"""
        return Image.BUFFER

    def buffer(self):
        """ Get image buffer 
        
        Returns:
            A memory buffer containing the encoded image.
        """
        length = c_int()
        format = c_int()
        data = trax_image_get_buffer(self.reference, byref(length), byref(format))
        return string_at(data, length.value)
