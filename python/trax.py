"""
\file trax.py

@brief Python implementation of the TraX sever

@author Alessio Dore

@date 2016
"""

import socket
import sys
import logging as log

TRAX_LOCALHOST = '127.0.0.1'
TRAX_DEFAULT_PORT = 9999

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

class SocketServer(object):
    """ Base TraX socket server """
    def __init__(self, port=None):
        """ Constructor
        
        Args: 
            port: if None use TRAX_DEFAULT_PORT
        """
        self.port = TRAX_DEFAULT_PORT
        if port:
            self.port  = port
        self.host = TRAX_LOCALHOST 
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        log.info('Socket created')
        
        # Connect to localhost
        try:
            self.socket.connect((TRAX_LOCALHOST,TRAX_DEFAULT_PORT))
            log.info('Server connected')
        except socket.error as msg:
            log.error('Connection failed. Error Code : {}\nMessage: {}'.format(str(msg[0]), msg[1]))
            sys.exit()    

        # to store single messages if more than one read from socket
        self.receivedMsgs = []  

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
            
        # @fixme: Not sure if I need to split the msg in chucks of TRAX_BUFFER_SIZE      
        #for i in xrange(int(len(msg)/TRAX_BUFFER_SIZE)+1):
            #if i*TRAX_BUFFER_SIZE == len(msg):
                #break
            #self.socket.send(msg[i*TRAX_BUFFER_SIZE:max((i+1)*TRAX_BUFFER_SIZE,len(msg))])
            
        log.info('Sending: {}'.format(msg))
        self.socket.send(msg)
        return


    def _read_message(self):
        """ Read socket message and parse it
         
        Returns:  
            msgArgs: list of message arguments
        """   
        if not len(self.receivedMsgs) or not '\n' in self.receivedMsgs[0]:
            msg = '' if not len(self.receivedMsgs) else self.receivedMsgs[0]
            while True:
                msg += self.socket.recv(1024)
                if msg is None or not isinstance(msg,str):
                    return None, None
                if len(msg) == 0:
                    log.info('Empty msg')
                    continue
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

class TraxServer(SocketServer):
    """ Python TraX Server 
    """
    def __init__(self, options, port = None, verbose = False):
        """ Constructor
        
        Args: 
            options: message type identifier
            port: port
        """
        if verbose:
            log.basicConfig(format="%(levelname)s: %(message)s", level=log.DEBUG)
        else:
            log.basicConfig(format="%(levelname)s: %(message)s")
            
        self.options = options
        super(TraxServer, self).__init__(port)

    def __enter__(self):
        """ To support instantiation with 'with' statement """
        return self

    def __exit__(self,*args,**kwargs):
        """ Destructor used by 'with' statement. Close socket connection """
        self.socket.close() 
        return

    def trax_server_setup(self):
        """ Send hello msg with options to TraX client """
        properties = [self.options.getAttrStr(prop) for prop in self.options.__dict__.keys()]
        self._write_message(TRAX_HELLO, [], properties)
        return
        
    def trax_server_wait(self):
        """ Wait for client msg. Recognize them and parse them when received """
        
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
        self._write_message(TRAX_STATUS, region, properties)

class TraxServerOptions(object):
    """ TraX server options """
    def __init__(self, name, identifier, region, image, version = TRAX_VERSION):
        """ Constructor
        
        Args:
            name: name of the tracker
            identifier: identifier of the current implementation
            region: region format. Supported: TRAX_REGION_RECTANGLE,TRAX_REGION_POLYGON  
            image: image format. Supported: TRAX_IMAGE_PATH 
            version: version of the TraX protocol
        """
        self.name = name
        self.identifier = identifier
        # other formats not implemented yet
        assert(region in [TRAX_REGION_RECTANGLE,TRAX_REGION_POLYGON])
        assert(image == TRAX_IMAGE_PATH)
        self.region = region
        self.image = image
        self.version = version
    
    def getAttrStr(self, attrName):
        """ Return the string to send to the client for each attribute of the class
        
        Args:
            attrName: name of the class attribute
        Returns:
            String with attribute to send to the client. Format: "trax.<attr name>=<attr val>"
        """
        return '"trax.{}={}"'.format(attrName, getattr(self, attrName))
