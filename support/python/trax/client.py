"""
Bindings for the TraX sever.
"""

import os
import time
import collections

from ctypes import byref, cast

from . import TraxException, TraxStatus, Properties, HandleWrapper, ImageListWrapper, \
    ConsoleLogger, FileLogger, ProxyLogger
    
from .internal import \
    trax_image_list_set, trax_client_initialize, \
    trax_client_wait, trax_region_p, trax_properties_p, \
    trax_image_create_path, trax_client_frame, \
    trax_client_setup_file, trax_client_setup_socket, \
    trax_image_list_create, trax_logger_setup, trax_logger, \
    trax_properties_create, trax_terminate, trax_get_error, \
    trax_is_alive
from .image import ImageChannel, Image
from .region import Region

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

    def __init__(self, stream=None, timeout=30, log=False):

        if isinstance(log, bool) and log:
            self._logger = ConsoleLogger()
        elif isinstance(log, str):
            self._logger = FileLogger(log)
        elif callable(log):
            self._logger = ProxyLogger(log)
        else:
            self._logger = None

        self._clogger = trax_logger(self._logger) if not self._logger is None else None

        logger = trax_logger_setup(self._clogger, 0, 0)

        if isinstance(stream, tuple):

            assert(len(stream) == 2)

            handle = trax_client_setup_file(
                stream[1],
                stream[0],
                logger)

        elif isinstance(stream, int):

            handle = trax_client_setup_socket(
                stream,
                timeout,
                logger)

        else:
            raise TraxException("Invalid parameters")

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if not handle:
            raise TraxException("Unable to connect to tracker")

        if trax_is_alive(handle) == 0:
            message = trax_get_error(handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when connecting to tracker: {}".format(message))

        self._handle = HandleWrapper(handle)

        metadata = self._handle.reference.contents.metadata

        self._format_region = Region.decode_list(metadata.contents.format_region)
        self._format_image = Image.decode_list(metadata.contents.format_image)
        self._channels = ImageChannel.decode_list(metadata.contents.channels)
        
        self._tracker_name = metadata.contents.tracker_name.decode("utf-8") \
            if not metadata.contents.tracker_name is None else ""
        self._tracker_family = metadata.contents.tracker_family.decode("utf-8") \
            if not metadata.contents.tracker_family is None else ""
        self._tracker_description = metadata.contents.tracker_description.decode("utf-8") \
            if not metadata.contents.tracker_description is None else ""

        custom = Properties(metadata.contents.custom, False)

        self._custom = custom.dict()

    @property
    def channels(self):
        return self._channels

    @property
    def image_formats(self):
        return self._format_image

    @property
    def region_formats(self):
        return self._format_region

    @property
    def tracker_name(self):
        return self._tracker_name

    @property
    def tracker_family(self):
        return self._tracker_family

    @property
    def tracker_description(self):
        return self._tracker_description

    def get(self, key):
        if key in self._custom:
            return self._custom[key]
        return None

    def initialize(self, images, region, properties):

        timage = wrap_images(images)
        tregion = region.reference
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_initialize(self._handle.reference, timage.reference, tregion, tproperties.reference))

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when initializing tracker: {}".format(message))

        tregion = trax_region_p()
        properties = Properties()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tregion), properties.reference))

        elapsed = time.time() - start

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            raise TraxException("Exception when waiting for response")
        if status == TraxStatus.QUIT:
            reason = properties.get("trax.reason", None)
            if reason is None:
                raise TraxException("Server terminated the session")
            else:
                raise TraxException("Server terminated the session: {}".format(reason))

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when waiting for response: {}".format(message))

        region = Region.wrap(tregion)

        return region, properties, elapsed

    def frame(self, images, properties = dict()):

        timage = wrap_images(images)
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_frame(self._handle.reference, timage.reference, tproperties.reference))

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when sending frame to tracker: {}".format(message))

        tregion = trax_region_p()
        properties = Properties()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tregion), properties.reference))

        elapsed = time.time() - start

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when waiting for response: {}".format(message))
        if status == TraxStatus.QUIT:
            reason = properties.get("trax.reason", None)
            if reason is None:
                raise TraxException("Server terminated the session")
            else:
                raise TraxException("Server terminated the session: {}".format(reason))
            
        region = Region.wrap(tregion)

        return region, properties, elapsed

    def quit(self, reason=None):
        """ Sends quit message and end terminates communication. """
        if not reason is None:
            trax_terminate(self._handle.reference, reason.encode('utf-8'))
        else:
            trax_terminate(self._handle.reference, None)

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        self._handle = None

