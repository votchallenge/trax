
from .wrapper import PropertiesWrapper
from .internal import *

class TraxException(Exception):
    pass

class TraxStatus(object):
    """ The message type container class """
    ERROR = "error"
    OK = "ok"
    HELLO = "hello"
    INITIALIZE = "initialize"
    FRAME = "frame"
    QUIT = "quit"
    STATE = "state"

    @staticmethod
    def decode(intcode):
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


class Properties(object):

    def __init__(self, data=None):
        if not data:
            self._ref = PropertiesWrapper(trax_properties_create())
        elif isinstance(data, trax_properties_p):
            self._ref = PropertiesWrapper(data)
        elif isinstance(data, dict):
            self._ref = PropertiesWrapper(trax_properties_create())
            for key, value in data.items():
                trax_properties_set(self._ref.reference(), key, str(value))

    def __getitem__(self, key):
        return trax_properties_get(self._ref.reference(), key)

    def __setitem__(self, key, value):
        trax_properties_set(self._ref.reference(), key, str(value))

    def get(self, key, default = None):
        value = trax_properties_get(self._ref.reference(), key)
        if value == None:
            return default
        return value

    def set(self, key, value):
        trax_properties_set(self._ref.reference(), key, str(value))

from .server import Server
from .region import *
from .image import *

__all__ = \
    ['TraxException', 'FileImage', 'URLImage', 'Server',
    'MemoryImage', 'BufferImage', 'Region', 'Image', 'Polygon',
    'Rectangle', 'Special', 'TraxStatus', 'Properties']
