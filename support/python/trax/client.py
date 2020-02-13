"""
Bindings for the TraX sever.
"""

import os
import time
import collections

from ctypes import byref, cast

from . import TraxException, TraxStatus, Properties, HandleWrapper, ImageListWrapper, ConsoleLogger, FileLogger
from .internal import \
    trax_image_list_set, trax_client_initialize, \
    trax_client_wait, trax_region_p, trax_properties_p, \
    trax_image_create_path, trax_client_frame, \
    trax_client_setup_file, trax_client_setup_socket, \
    trax_image_list_create, trax_logger_setup, trax_logger
from .image import ImageChannel, Image
from .region import Region

class Request(collections.namedtuple('Request', ['type', 'image', 'region', 'properties'])):

    """ A container class for client requests. Contains fileds type, image, region and parameters. """

def wrap_images(images):

    channels = [ImageChannel.COLOR, ImageChannel.DEPTH, ImageChannel.IR]
    tlist = ImageListWrapper(trax_image_list_create())

    for channel in channels:
        if not channel in images:
            continue

        trax_image_list_set(tlist.reference, images[channel].reference, ImageChannel.encode(channel))

    return tlist


class Client(object):

    """ TraX client."""

    def __init__(self, streams=None, port=None, timeout=None, log=False):

        if isinstance(log, bool) and log:
            self._logger = trax_logger(ConsoleLogger())
        elif isinstance(log, str):
            self._logger = trax_logger(FileLogger(log))
        else:
            self._logger = None

        assert(len(streams) == 2)

        logger = trax_logger_setup(self._logger, 0, 0)

        if (streams and not (port or timeout)):
            
            handle = trax_client_setup_file(
                streams[1],
                streams[0],
                logger)

        elif (port and timeout and not streams):

            handle = trax_client_setup_socket(
                port,
                timeout,
                logger)

        else:
            raise TraxException("Invalid parameters, use streams or port and timeout")

        if not handle:
            raise TraxException("Unable to connect to tracker")

        self._handle = HandleWrapper(handle)

        metadata = self._handle.reference.contents.metadata

        self._format_region = Region.decode_list(metadata.contents.format_region)
        self._format_image = Image.decode_list(metadata.contents.format_image)
        self._channels = ImageChannel.decode_list(metadata.contents.channels)
        
        self._tracker_name = metadata.contents.tracker_name.decode("utf-8")
        self._tracker_family = metadata.contents.tracker_family.decode("utf-8")
        self._tracker_description = metadata.contents.tracker_description.decode("utf-8")

    @property
    def channels(self):
        return self._channels

    def initialize(self, images, region, properties):

        timage = wrap_images(images)
        tregion = region.reference
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_initialize(self._handle.reference, timage.reference, tregion, tproperties.reference))

        if status == TraxStatus.ERROR:
            raise TraxException("Exception when initializing tracker")

        tregion = trax_region_p()
        tproperties = trax_properties_p()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tregion), tproperties))

        elapsed = time.time() - start

        if status == TraxStatus.ERROR:
            raise TraxException("Exception when waiting for response")
        if status == TraxStatus.QUIT:
            raise TraxException("Server terminated the session")

        region = Region.wrap(tregion)
        properties = Properties(tproperties)

        return region, properties, elapsed

    def frame(self, images, properties = dict()):

        timage = wrap_images(images)
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_frame(self._handle.reference, timage.reference, tproperties.reference))

        if status == TraxStatus.ERROR:
            raise TraxException("Exception when sending frame to tracker")

        tregion = trax_region_p()
        tproperties = trax_properties_p()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tregion), tproperties))

        elapsed = time.time() - start

        if status == TraxStatus.ERROR:
            raise TraxException("Exception when waiting for response")
        if status == TraxStatus.QUIT:
            raise TraxException("Server terminated the session")


        region = Region.wrap(tregion)
        properties = Properties(tproperties)

        return region, properties, elapsed

    def quit(self):
        """ Sends quit message and end terminates communication. """
        self._handle = None
