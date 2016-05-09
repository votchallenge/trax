"""
\file server.py

@brief Python implementation of the TraX sever

@author Alessio Dore, Luka Cehovin

@date 2016
"""

import os
import sys
import socket
import logging as log
import collections


from . import TraXError, MessageType
from .message import MessageParser
import trax.region
import trax.image

DEFAULT_HOST = '127.0.0.1'

TRAX_VERSION = 1

Request = collections.namedtuple('Request', ['type', 'image', 'region', 'parameters'])

class Server(MessageParser):
    """ TraX server implementation class."""
    def __init__(self, options, verbose=False):
        """ Constructor
        
        Args: 
            verbose: if True display log info
        """
        if verbose:
            log.basicConfig(format="%(levelname)s: %(message)s", level=log.DEBUG)
        else:
            log.basicConfig(format="%(levelname)s: %(message)s")      

        self.options = options

        if "TRAX_SOCKET" in os.environ:

            port = int(os.environ.get("TRAX_SOCKET"))
        
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            log.info('Socket created')        
            # Connect to localhost
            try:                   
                self.socket.connect((DEFAULT_HOST, port))              
                log.info('Server connected to client')   
            except socket.error as msg:
                log.error('Connection failed. Error Code: {}\nMessage: {}'.format(str(msg[0]), msg[1]))          
                raise TraXError("Unable to connect to client: {}\nMessage: {}".format(str(msg[0]), msg[1]))
        
            super(Server, self).__init__(os.fdopen(self.socket.fileno(), 'r'), os.fdopen(self.socket.fileno(), 'w'))

        else:
            fin = sys.stdin
            fout = sys.stdout

            env_in = int(os.environ.get("TRAX_IN", "-1"))
            env_out = int(os.environ.get("TRAX_OUT", "-1"))

            super(Server, self).__init__(
                os.fdopen(env_in, 'r') if env_in > 0 else fin,
                os.fdopen(env_out, 'w') if env_out > 0 else fout
            )

        self._setup()

    def _setup(self):
        """ Send hello message with capabilities to the TraX client """ 
 
        properties = { "trax.{}".format(prop) : getattr(self.options, prop) for prop in self.options.__dict__.keys()}         
        self._write_message(MessageType.HELLO, [], properties)     
        return
        
    def wait(self):
        """ Wait for client message request. Recognize it and parse them when received 
        
        Returns: 
            request: a request structure
        """
        
        message = self._read_message()    
        if message.type == None:
            return Request(MessageType.ERROR, None, None, None)

        elif message.type == MessageType.QUIT and len(message.arguments) == 0:
            log.info('Received quit message from client.')
            return Request(message.type, None, None, message.parameters)

        elif message.type == MessageType.INITIALIZE and len(message.arguments) == 2:
            log.info('Received initialize message.')

            image = trax.image.Image.parse(message.arguments[0])
            region = trax.region.Region.parse(message.arguments[1])        
            return Request(message.type, image, region, message.parameters)

        elif message.type == MessageType.FRAME and len(message.arguments) == 1:
            log.info('Received frame message.')

            image = trax.image.Image.parse(message.arguments[0])
            return Request(message.type, image, None, message.parameters)               

        else:
            return Request(MessageType.ERROR, None, None, None)

    def status(self, region, properties=None):
        """ Reply to client with a status region and optional properties
        
        Args: 
            region: Resulting region object.
            properties: Optional arguments as a dictionary.     
        """
        assert(isinstance(region, trax.region.Region))
        self._write_message(MessageType.STATUS, [region], properties)

    def __enter__(self):
        """ To support instantiation with 'with' statement """
        return self

    def __exit__(self,*args,**kwargs):
        """ Destructor used by 'with' statement. """  
        self.quit()

    def quit(self):
        self._close()
        if hasattr(self, 'socket'):
            self.socket.close()

class ServerOptions(object):
    """ TraX server options """
    def __init__(self, name, identifier, region, image, version=TRAX_VERSION):
        """ Constructor
        
        Args:
            name: name of the tracker
            identifier: identifier of the current implementation
            region: region format. Supported: REGION_RECTANGLE, REGION_POLYGON  
            image: image format. Supported: IMAGE_PATH 
            version: version of the TraX protocol
        """

        # other formats not implemented yet
        assert(region in [trax.region.RECTANGLE, trax.region.POLYGON])
        assert(image == trax.image.PATH)

        self.name = name
        self.identifier = identifier
        self.region = region
        self.image = image
        self.version = version
    
