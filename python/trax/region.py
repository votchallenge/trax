"""
\file region.py

@brief Python implementation of the TraX protocol - region classes

@author Alessio Dore, Luka Cehovin

@date 2016
"""

import os
import sys

SPECIAL = "special"
RECTANGLE = "rectangle"
POLYGON = "polygon"
MASK = "mask"

class Region(object):
    """ Base class for region """
    def __init__(self):
        pass
        
    @staticmethod
    def parse(string):
        tokens = map(float, string.strip('"').split(','))
        if len(tokens) == 4:
            return Rectangle(tokens[0], tokens[1], tokens[2], tokens[3])
        elif len(tokens) % 2 == 0 and len(tokens) > 4:
            return Polygon([(tokens[i],tokens[i+1]) for i in xrange(0,len(tokens),2)])
        return
            
class Rectangle(Region):
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
        super(Rectangle, self).__init__()
        self.type = RECTANGLE
        self.x, self.y, self.w, self.h = x, y, w, h

    def __str__(self):
        """ Create string from class to send to client """
        return '{},{},{},{}'.format(self.x, self.y, self.w, self.h)
        
class Polygon(Region):
    """ Polygon region 
    """
    def __init__(self, points):
        """
        Constructor
    
        Args: points list of points coordinates as tuples [(x1,y1), (x2,y2),...,(xN,yN)]  
        """
        super(Polygon, self).__init__()
        assert(isinstance(points, list))
        # do not allow empty list
        assert(reduce(lambda x,y: x and y, [isinstance(p,tuple) for p in points], False))
        self.count = len(points) 
        self.points = points
        self.type = POLYGON

    def __str__(self):
        """ Create string from class to send to client """
        return ','.join(['{},{}'.format(p[0],p[1]) for p in self.points])
        
            
