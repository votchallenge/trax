"""
\file __init__.py

@brief Python implementation of the TraX sever

@author Alessio Dore, Luka Cehovin

@date 2016
"""

class MessageType(object):
    ERROR = "error"
    HELLO = "hello"
    INITIALIZE = "initialize"
    FRAME = "frame"
    QUIT = "quit"
    STATUS = "status"

class TraXError(RuntimeError):
    pass

