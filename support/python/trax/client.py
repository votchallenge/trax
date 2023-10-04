"""
Bindings for the TraX client object.
"""

__all__ = \
    ['Client']

import time

from ctypes import byref, c_int

from . import TraxException, TraxStatus, Properties, HandleWrapper, \
    ConsoleLogger, FileLogger, ProxyLogger, trax_object_list_p
    
from . import wrap_images, wrap_objects, wrap_object_list, ObjectListWrapper

from ._ctypes import \
    trax_client_initialize, \
    trax_client_wait, trax_client_frame, \
    trax_client_setup_file, trax_client_setup_socket, \
    trax_logger_setup, trax_logger, trax_get_parameter, \
    trax_terminate, trax_get_error, trax_is_alive, TRAX_PARAMETER_MULTIOBJECT

from .image import ImageChannel, Image
from .region import Region

class Client(object):
    """ TraX client object. """

    def __init__(self, stream=None, timeout=30, log=False):
        """ Create a new client object.
        
        Args:
            stream: A tuple (host, port) or a port number to connect to.
            timeout: A timeout in seconds.
            log: A boolean value or a string. If True, the log is written to the console. If False, no log is written. If a string, the log is written to the file with the given name.
        """

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

        value = c_int(value=0)   
        trax_get_parameter(handle, TRAX_PARAMETER_MULTIOBJECT, byref(value))

        self._custom["multiobject"] = bool(value.value)

    @property
    def channels(self):
        """ List of supported channels.
        
        Returns:
            A list of ImageChannel objects.
        """
        return self._channels

    @property
    def image_formats(self):
        return self._format_image

    @property
    def region_formats(self):
        """ List of supported region formats.
        
        Returns:
            A list of Region objects.
        """
        return self._format_region

    @property
    def tracker_name(self) -> str:
        """ Tracker name.
        
        Returns:
            A string with the tracker name.
        """
        return self._tracker_name

    @property
    def tracker_family(self) -> str:
        """ Tracker family property.
        
        Returns:
            A string with the tracker family.
        """
        return self._tracker_family

    @property
    def tracker_description(self) -> str:
        """ Tracker description property.
        
        Returns:
            A string with the tracker description.
        """
        return self._tracker_description

    def get(self, key) -> str:
        """ Get a custom property provided by the tracker.
        
        Args:
            key: A string with the property name.
            
        Returns:
            A string with the property value.
        """
        if key in self._custom:
            return self._custom[key]
        return None

    def initialize(self, images, objects, properties):
        """ Initialize the tracker. The response is returned as a tuple of objects and elapsed time.
        
        Args:
            images (list of Image): List of images to be sent to the tracker.
            objects (list of Region): List of objects to be sent to the tracker.
            properties (dict): Dictionary of properties to be sent to the tracker.
        """

        timage = wrap_images(images)
        tobjects = wrap_objects(objects)
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_initialize(self._handle.reference, timage.reference, tobjects.reference, tproperties.reference))

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when initializing tracker: {}".format(message))

        tobjects = trax_object_list_p()
        properties = Properties()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tobjects), properties.reference))

        elapsed = time.time() - start

        tobjects = ObjectListWrapper(tobjects)

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

        return wrap_object_list(tobjects.reference), elapsed

    def frame(self, images, properties = dict(), objects = None):
        """ Send a frame to the tracker and wait for the response. The response is returned as a tuple of objects and elapsed time.
        The objects are returned as a list of tuples of region and property objects.
        The elapsed time is the time it took for the tracker to process the frame in seconds.
        
        Args:
            images (list of Image): The list of images to send to the tracker.
            properties (dict, optional): The properties to send to the tracker.
            objects (list of Region, optional): The list of regions to send to the tracker.

        Returns:
            list: The tuple of objects and elapsed time.
        """

        timage = wrap_images(images)
        tobjects = wrap_objects(objects)
        tproperties = Properties(properties)

        status = TraxStatus.decode(trax_client_frame(self._handle.reference, timage.reference, tobjects.reference, tproperties.reference))

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")

        if status == TraxStatus.ERROR:
            message = trax_get_error(self._handle.reference)
            message = message.decode('utf-8') if not message is None else "Unknown"
            raise TraxException("Exception when sending frame to tracker: {}".format(message))

        tobjects = trax_object_list_p()
        properties = Properties()

        start = time.time()

        status = TraxStatus.decode(trax_client_wait(self._handle.reference, byref(tobjects), properties.reference))

        elapsed = time.time() - start

        tobjects = ObjectListWrapper(tobjects)

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
            
        return wrap_object_list(tobjects.reference), elapsed

    def quit(self, reason: str = None):
        """ Sends quit message and end terminates communication.
        
        Args:
            reason (str): Optional reason for quitting.
        """
        if not reason is None:
            trax_terminate(self._handle.reference, reason.encode('utf-8'))
        else:
            trax_terminate(self._handle.reference, None)

        if self._logger and self._logger.interrupted:
            raise KeyboardInterrupt("Interrupted by user in log callback")
