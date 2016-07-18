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

def convert(region, to):

    if to == RECTANGLE:

        if isinstance(region, Rectangle):
            return region.copy()
        elif isinstance(region, Polygon):
            top = sys.float_info.min
            bottom = sys.float_info.max
            left = sys.float_info.min
            right = sys.float_info.max

            for point in region.points: 
                top = min(top, point[1])
                bottom = max(bottom, point[1])
                left = min(left, point[0])
                right = max(right, point[0])

            return Rectangle(left, top, right - left, bottom - top)

        else:
            return None  
    if to == POLYGON:

        if isinstance(region, Rectangle):
            points = []
            points.append((region.x, region.y))
            points.append((region.x + region.width, region.y))
            points.append((region.x + region.width, region.y + region.height))
            points.append((region.x, region.y + region.height))
            return Polygon(points)

        elif isinstance(region, Polygon):
            return region.copy()
        else:
            return None  

    elif to == SPECIAL:
        if isinstance(region, Special):
            return region.copy()
        else:
            return Special()

    return None

def parse(string):
    tokens = map(float, string.split(','))
    if len(tokens) == 1:
        return Special(tokens[0])
    if len(tokens) == 4:
        return Rectangle(tokens[0], tokens[1], tokens[2], tokens[3])
    elif len(tokens) % 2 == 0 and len(tokens) > 4:
        return Polygon([(tokens[i],tokens[i+1]) for i in xrange(0,len(tokens),2)])
    return None

class Region(object):
    """ Base class for region """
    def __init__(self):
        pass

class Special(Region):
    """ Special region 
    """
    def __init__(self, code):
        """ Constructor
        
        Args:
            code: special code
        """
        super(Special, self).__init__()
        self.type = SPECIAL
        self.code = int(code)

    def __str__(self):
        """ Create string from class to send to client """
        return '{}'.format(self.code)
 
class Rectangle(Region):
    """ Rectangle region 
    """
    def __init__(self, x=0, y=0, width=0, height=0):
        """ Constructor
        
        Args:
            x: top left x coord of the rectangle region
            y: top left y coord of the rectangle region
            w: width of the rectangle region
            h: height of the rectangle region
        """
        super(Rectangle, self).__init__()
        self.type = RECTANGLE
        self.x, self.y, self.width, self.height = x, y, width, height

    def __str__(self):
        """ Create string from class to send to client """
        return '{},{},{},{}'.format(self.x, self.y, self.width, self.height)
        
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
        
            
