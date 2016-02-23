"""
\file trax.py

@brief Python implementation of the TraX sever

@author Alessio Dore

@date 2016
"""

import os
import sys
import socket
import logging as log

TRAX_LOCALHOST = '127.0.0.1'
TRAX_DEFAULT_PORT = 9090

TRAX_VERSION = 1

TRAX_PREFIX = '@@TRAX:'
TRAX_HELLO_STR = 'hello'
TRAX_INITIALIZE_STR = 'initialize'
TRAX_FRAME_STR = 'frame'
TRAX_QUIT_STR = 'quit'
TRAX_STATUS_STR = 'status'

TRAX_BUFFER_SIZE = 128

TRAX_ERROR, TRAX_HELLO, TRAX_INITIALIZE, TRAX_FRAME, TRAX_QUIT, TRAX_STATUS = range(-1,5)
TRAX_REGION_SPECIAL, TRAX_REGION_RECTANGLE, TRAX_REGION_POLYGON, TRAX_REGION_MASK = range(4)
TRAX_IMAGE_PATH, TRAX_IMAGE_URL, TRAX_IMAGE_DATA = range(3)

TRAX_STREAM_FILES, TRAX_STREAM_SOCKET = range(1,3)

class Server(object):
    """ Base TraX server. Just interface, methods to implement in derived classes """
    def __init__(self, comm_type=TRAX_STREAM_FILES, port=None, verbose=False):
        """ Constructor
        
        Args: 
            comm_type: type of communication between client and server. 
                       Available: TRAX_STREAM_FILES (stdin and stdout pipes) and TRAX_STREAM_SOCKET (socket)
            port: if None use TRAX_DEFAULT_PORT
            verbose: if True display log info
        """
        if verbose:
            log.basicConfig(format="%(levelname)s: %(message)s", level=log.DEBUG)
        else:
            log.basicConfig(format="%(levelname)s: %(message)s")      
        
        self.comm_type = comm_type
        # communication functions for socket and pipe    
        if self.comm_type == TRAX_STREAM_FILES:
            def write_to_stdout(msg):
                sys.stdout.write(msg)
                sys.stdout.flush()
            self._send_msg = write_to_stdout
            self._recv_msg = sys.stdin.readline
            self.nRec = None # not to specify for readlines
        elif comm_type == TRAX_STREAM_SOCKET:
            self._send_msg = self.socket.send
            self._recv_msg = self.socket.recv
            self.nRec = 1024 # bytes 
        else:
            log.error('Unknow communication type. Exit!')
            sys.exit(-1)
        
        # to store single messages if more than one read from socket
        self.receivedMsgs = []          
        
    def __enter__(self):
        """ To support instantiation with 'with' statement """
        return self

    def __exit__(self,*args,**kwargs):
        """ Destructor used by 'with' statement. Close stream or socket connection depending on the server """  
        if self.comm_type == TRAX_STREAM_FILES:
            sys.stdin.close()
            sys.stdout.close()
        elif self.comm_type == TRAX_STREAM_SOCKET:
            self.socket.close()
        return        

    def _write_message(self, msgType, arguments, properties):
        """ Create the message string and send it
        
        Args: 
            msgType: message type identifier
            arguments: message arguments  
            properties: optional arguments. Format: "key:value"
        """
        assert(isinstance(arguments, list))
        assert(isinstance(properties, list) or properties is None)
        
        msg = '{}'.format(TRAX_PREFIX)
        if msgType == TRAX_HELLO:        
            msg += TRAX_HELLO_STR
        elif msgType == TRAX_STATUS:        
            msg += TRAX_STATUS_STR
        elif msgType == TRAX_QUIT:        
            msg += TRAX_QUIT_STR      
        
        # arguments
        for arg in arguments:
            msg += ' '
            if isinstance(arg,str) and ' ' in arg:
                msg += '\"' + arg + '\"' 
            else:
                msg += arg      
                
        # optional arguments
        if not properties: properties = [] 
        for prop in properties:
            msg += ' '
            if msg[-1:] != ' ':
                msg += ' '
            msg += prop
        # end of msg    
        msg += '\n'
            
        log.info('Sending: {}'.format(msg))
        self._send_msg(msg)
        return

    def _read_message(self):
        """ Read socket message and parse it
         
        Returns:  
            msgArgs: list of message arguments
        """            
        if not len(self.receivedMsgs) or (not TRAX_QUIT_STR in self.receivedMsgs[0] and not '\n' in self.receivedMsgs[0]):
            msg = '' if not len(self.receivedMsgs) else self.receivedMsgs[0]           
            while True:    
                args = [self.nRec] if self.comm_type==TRAX_STREAM_SOCKET else []
                msg += self._recv_msg(*args)               
                if msg is None or not isinstance(msg,str):                  
                    return None, None
                if len(msg) == 0:
                    log.info('Empty msg')                     
                    return None, None
                else:
                    if not '\n' in msg and not TRAX_QUIT_STR in msg:
                        log.info('Incomplete msg: {}'.format(msg))
                        continue
                    tmpMsgs = [m for m in msg.split('\n') if len(m) > 0]
                    msg = tmpMsgs[0]
                    self.receivedMsgs += tmpMsgs[1:]
                    break
        else:
            msg = self.receivedMsgs[0]
            self.receivedMsgs = self.receivedMsgs[1:]
            
        if msg[:len(TRAX_PREFIX)] != TRAX_PREFIX:
            log.warning('Message is not in TraX format: {}'.format(msg))
            return None, None
        
        msgArgs = [m for m in msg[len(TRAX_PREFIX):].split(' ') if len(m) > 0]
        
        if not msgArgs[0] in [TRAX_INITIALIZE_STR, TRAX_FRAME_STR, TRAX_QUIT_STR]:
            log.warning('The message is not recognized: {}'.format(msg))
            return None, None
        
        if msgArgs[0] == TRAX_INITIALIZE_STR: msgType = TRAX_INITIALIZE
        elif msgArgs[0] == TRAX_FRAME_STR: msgType = TRAX_FRAME
        elif msgArgs[0] == TRAX_QUIT_STR: msgType = TRAX_QUIT
        else:
            log.warning('The message is not recognized: {}'.format(msg))
            return None, None            
        msgArgs = self._fixArgs(msgArgs)

        return msgType, msgArgs

    def _fixArgs(self, msgArgs):
        """ Parse msgArgs list to merge string with spaces withing quotes 
            IMPORTANT: It assumes that arguments with white space are enclosed in double quotes
        """
        # remove the msg type str
        msgArgs = msgArgs[1:] if len(msgArgs) > 1 else []

        if len(msgArgs) == 0:
            return msgArgs

        # parse msgArgs list to merge string with spaces within quotes
        fixedMsgArgs = []
        skipToIdx = 0
        for idx, msgArg in enumerate(msgArgs):
            if skipToIdx > idx:
                continue
            if msgArg[0] == '"' and msgArg[-1] != '"':
                for i in range(idx+1,len(msgArgs)):
                    msgArg+= ' ' + msgArgs[i]
                    if msgArg[-1] == '"':
                        skipToIdx = i+1
                        fixedMsgArgs.append(msgArg)
                        break
            else:
                fixedMsgArgs.append(msgArg)  
        return fixedMsgArgs

        
class SocketServer(Server):
    """ Base TraX socket server """
    def __init__(self, comm_type=TRAX_STREAM_SOCKET, port=None, verbose=False):
        """ Constructor
        
        Args: 
            comm_type: type of communication between client and server. 
                       Available: TRAX_STREAM_FILES (stdin and stdout pipes) and TRAX_STREAM_SOCKET (socket)
            port: if None use TRAX_DEFAULT_PORT
            verbose: if True display log info
        """
        self.port = TRAX_DEFAULT_PORT
        if port:
            self.port  = port        
        self.host = TRAX_LOCALHOST    
        
        if comm_type == TRAX_STREAM_SOCKET:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            log.info('Socket created')        
            # Connect to localhost
            try:                   
                self.socket.connect((TRAX_LOCALHOST, TRAX_DEFAULT_PORT))                 
                log.info('Server connected')   
            except socket.error as msg:
                log.error('Connection failed. Error Code : {}\nMessage: {}'.format(str(msg[0]), msg[1]))          
                sys.exit()    
        
        super(SocketServer, self).__init__(comm_type, self.port, verbose) 

class FilesServer(Server):
    """ Base TraX server using file pipes stdout stdin
    """
    def __init__(self, comm_type=TRAX_STREAM_FILES, port=None, verbose=False):
        """ Constructor
        
        Args: 
            comm_type: type of communication between client and server. 
                       Available: TRAX_STREAM_FILES (stdin and stdout pipes) and TRAX_STREAM_SOCKET (socket)
            port: if None use TRAX_DEFAULT_PORT
            verbose: if True display log info
        """
        super(FilesServer, self).__init__(comm_type, port, verbose)        

class TraxServer(FilesServer, SocketServer):
    """ Python TraX Server 
    """
    def __init__(self, options, port=None, verbose=False):
        """ Constructor
        
        Args: 
            options: TraX server options 
            port: port
            verbose: if True display log info
        """      
        self.options = options
        
        # files stream server
        if options.comm_type == TRAX_STREAM_FILES:
            super(TraxServer, self).__init__(options.comm_type, None, verbose)       
        # socket server
        elif options.comm_type == TRAX_STREAM_SOCKET:
            port = port if port else TRAX_DEFAULT_PORT
            super(TraxServer, self).__init__(options.comm_type, port, verbose)
        else:
            log.error('Unknown communication type. Exit!')
            sys.exit()

    def trax_server_setup(self):
        """ Send hello msg with options to TraX client """  
        properties = [self.options.getAttrStr(prop) for prop in self.options.__dict__.keys()]              
        self._write_message(TRAX_HELLO, [], properties)     
        return
        
    def trax_server_wait(self):
        """ Wait for client msg. Recognize them and parse them when received 
        
        Returns: 
            msgType: type of the message (TRAX_ERROR, TRAX_HELLO, TRAX_INITIALIZE, TRAX_FRAME, TRAX_QUIT, TRAX_STATUS)
            msgArgs: returned arguments in a list    
        """
        
        msgType, msgArgs = self._read_message()    
        if msgType == None:
            return TRAX_ERROR, []          
        # quit msg
        elif msgType == TRAX_QUIT and len(msgArgs) == 0:
            log.info('Received quit msg from client.')
            return TRAX_QUIT, []  
        # initialize msg
        elif msgType == TRAX_INITIALIZE and len(msgArgs) == 2:
            log.info('Received initialize msg. Args = {}'.format(msgArgs))
            # validate args?
            imgPath = msgArgs[0]
            traxRegion = msgArgs[1]        
            return msgType, msgArgs
        # frame msg
        elif msgType == TRAX_FRAME and len(msgArgs) == 1:
            log.info('Received frame msg. Arg = {}'.format(msgArgs))
            # validate args?
            imgPath = msgArgs[0]       
            return msgType, msgArgs                  
        # error msg
        else:
            return TRAX_ERROR, []

    def trax_server_reply(self, region, properties=None):
        """ Reply to client 
        
        Args: 
            region: tracked region. Format: "x1,y1,x2,y2,..." (rect: tl_x,tly,w,h) 
            properties: optional arguments. Format: "key:value"       
        """
        assert(isinstance(region, str))
        self._write_message(TRAX_STATUS, [region], properties)

class TraxServerOptions(object):
    """ TraX server options """
    def __init__(self, name, identifier, region, image, comm_type=TRAX_STREAM_FILES, version=TRAX_VERSION):
        """ Constructor
        
        Args:
            name: name of the tracker
            identifier: identifier of the current implementation
            region: region format. Supported: TRAX_REGION_RECTANGLE,TRAX_REGION_POLYGON  
            image: image format. Supported: TRAX_IMAGE_PATH 
            comm_type: TRAX_STREAM_FILES use pipe (stdin and stdout) - TRAX_STREAM_SOCKET use socket
            version: version of the TraX protocol
        """
        self.name = name
        self.identifier = identifier
        # other formats not implemented yet
        assert(region in [TRAX_REGION_RECTANGLE, TRAX_REGION_POLYGON])
        self.region = region
        assert(image == TRAX_IMAGE_PATH)
        self.image = image
        self.version = version
        assert(comm_type in [TRAX_STREAM_FILES, TRAX_STREAM_SOCKET])
        self.comm_type = comm_type
    
    def getAttrStr(self, attrName):
        """ Return the string to send to the client for each attribute of the class
        
        Args:
            attrName: name of the class attribute
        Returns:
            String with attribute to send to the client. Format: "trax.<attr name>=<attr val>"
        """
        return '"trax.{}={}"'.format(attrName, getattr(self, attrName))

class trax_region(object):
    """ Base class for vot region """
    def __init__(self):
        pass
        
    def parseRegionStr(self, regionStr):
        """ In derived classes implement method to parse region string """
        return
            
class trax_region_rect(trax_region):
    """ Rectangle region 
    """
    def __init__(self, x=0, y=0, w=0, h=0):
        """ Constructor
        
        Args:
            x: top left x coord of the rectangle region
            y: top left y coord of the rectangle region
            w: width of the rectangle region
            h: height of the rectangle region
        """
        super(trax_region_rect, self).__init__()
        self.regionType = TRAX_REGION_RECTANGLE
        self.x, self.y, self.w, self.h = x, y, w, h

    def __str__(self):
        """ Create string from class to send to client """
        return '{},{},{},{}'.format(self.x, self.y, self.w, self.h)
        
    def parseRegionStr(self, regionStr):
        """ Parse region string to get x, y, w, h """
        self.x, self.y, self.w, self.h = map(float, regionStr.strip('"').split(','))   
            
class trax_region_poly(trax_region):
    """ Polygon region 
    """
    def __init__(self, points):
        """
        Constructor
    
        Args: points list of points coordinates as tuples [(x1,y1), (x2,y2),...,(xN,yN)]  
        """
        super(trax_region_poly, self).__init__()
        assert(isinstance(points, list))
        # do not allow empty list
        assert(reduce(lambda x,y: x and y, [isinstance(p,tuple) for p in points], False))
        self.count = len(points) 
        self.points = points

    def __str__(self):
        """ Create string from class to send to client """
        return ','.join(['{},{}'.format(p[0],p[1]) for p in self.points])
        
    def parseRegionStr(self, regionStr):
        """ Parse polygon string to get a list of points (x,y) """
        pointsFlat = map(float, regionStr.strip('"').split(','))
        assert(len(pointsFlat)%2==0)
        self.count = pointsFlat/2
        self.points = [(pointsFlat[i],pointsFlat[i+1]) for i in xrange(0,len(pointsFlat),2)]
            