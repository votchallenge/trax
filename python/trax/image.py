"""
\file image.py

@brief Python implementation of the TraX protocol - image classes

@author Luka Cehovin

@date 2016
"""

import os
import sys

PATH = "path"
URL = "url"
DATA = "data"
MEMORY = "memory"

def parse(string):
    """ In derived classes implement method to parse image string """
    return FilePath(string)

class Image(object):
    """ Base class for image """
    def __init__(self):
        pass

class FilePath(Image):
    """ Image saved in a local file """
    def __init__(self, path=None):
        self.path = path
        self.type = PATH
        
    def __str__(self):
        """ Create string from image data """
        return self.path

