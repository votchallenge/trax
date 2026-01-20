import unittest
import numpy as np
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from trax.region import Region, Special, Rectangle, Polygon, Mask, Point


class TestRegionEncoding(unittest.TestCase):
    """Test region format encoding and decoding"""

    def test_encode_special(self):
        """Test encoding SPECIAL format"""
        self.assertEqual(Region.encode(Region.SPECIAL), 1)

    def test_encode_rectangle(self):
        """Test encoding RECTANGLE format"""
        self.assertEqual(Region.encode(Region.RECTANGLE), 2)

    def test_encode_polygon(self):
        """Test encoding POLYGON format"""
        self.assertEqual(Region.encode(Region.POLYGON), 4)

    def test_encode_mask(self):
        """Test encoding MASK format"""
        self.assertEqual(Region.encode(Region.MASK), 8)

    def test_encode_point(self):
        """Test encoding POINT format"""
        self.assertEqual(Region.encode(Region.POINT), 16)

    def test_encode_invalid(self):
        """Test encoding invalid format raises exception"""
        with self.assertRaises(IndexError):
            Region.encode("invalid_format")

    def test_decode_list_single(self):
        """Test decoding single format"""
        self.assertEqual(Region.decode_list(1), [Region.SPECIAL])
        self.assertEqual(Region.decode_list(2), [Region.RECTANGLE])
        self.assertEqual(Region.decode_list(4), [Region.POLYGON])
        self.assertEqual(Region.decode_list(8), [Region.MASK])
        self.assertEqual(Region.decode_list(16), [Region.POINT])

    def test_decode_list_multiple(self):
        """Test decoding multiple formats"""
        self.assertEqual(Region.decode_list(3), [Region.SPECIAL, Region.RECTANGLE])
        self.assertEqual(Region.decode_list(7), [Region.SPECIAL, Region.RECTANGLE, Region.POLYGON])
        self.assertEqual(Region.decode_list(31), 
                         [Region.SPECIAL, Region.RECTANGLE, Region.POLYGON, Region.MASK, Region.POINT])

    def test_encode_list(self):
        """Test encoding list of formats"""
        self.assertEqual(Region.encode_list([Region.SPECIAL]), 1)
        self.assertEqual(Region.encode_list([Region.RECTANGLE, Region.POLYGON]), 6)
        self.assertEqual(Region.encode_list([Region.SPECIAL, Region.RECTANGLE, Region.POLYGON, Region.MASK, Region.POINT]), 31)

    def test_encode_decode_roundtrip(self):
        """Test encoding and decoding roundtrip"""
        formats = [Region.SPECIAL, Region.RECTANGLE, Region.POLYGON, Region.MASK, Region.POINT]
        encoded = Region.encode_list(formats)
        decoded = Region.decode_list(encoded)
        self.assertEqual(decoded, formats)


class TestSpecialRegion(unittest.TestCase):
    """Test Special region creation and properties"""

    def test_create_special(self):
        """Test creating a special region"""
        special = Special.create(0)
        self.assertIsNotNone(special)
        self.assertEqual(special.type, Region.SPECIAL)

    def test_special_code(self):
        """Test special region code property"""
        special = Special.create(5)
        self.assertEqual(special.code, 5)

    def test_special_string_representation(self):
        """Test special region string representation"""
        special = Special.create(1)
        self.assertIn('Special region', str(special))
        self.assertIn('code', str(special))


class TestRectangleRegion(unittest.TestCase):
    """Test Rectangle region creation and properties"""

    def test_create_rectangle_default(self):
        """Test creating a rectangle with default values"""
        rect = Rectangle.create()
        self.assertIsNotNone(rect)
        self.assertEqual(rect.type, Region.RECTANGLE)

    def test_create_rectangle_with_values(self):
        """Test creating a rectangle with specific values"""
        rect = Rectangle.create(10, 20, 100, 50)
        self.assertIsNotNone(rect)

    def test_rectangle_bounds(self):
        """Test rectangle bounds method"""
        x, y, w, h = 10.5, 20.5, 100.0, 50.0
        rect = Rectangle.create(x, y, w, h)
        bounds = rect.bounds()
        self.assertAlmostEqual(bounds[0], x, places=1)
        self.assertAlmostEqual(bounds[1], y, places=1)
        self.assertAlmostEqual(bounds[2], w, places=1)
        self.assertAlmostEqual(bounds[3], h, places=1)

    def test_rectangle_string_representation(self):
        """Test rectangle string representation"""
        rect = Rectangle.create(10, 20, 100, 50)
        self.assertIn('Rectangle', str(rect))
        self.assertIn('10', str(rect))
        self.assertIn('20', str(rect))
        self.assertIn('100', str(rect))
        self.assertIn('50', str(rect))


class TestPolygonRegion(unittest.TestCase):
    """Test Polygon region creation and properties"""

    def test_create_polygon(self):
        """Test creating a polygon"""
        points = [(0, 0), (100, 0), (100, 100), (0, 100)]
        polygon = Polygon.create(points)
        self.assertIsNotNone(polygon)
        self.assertEqual(polygon.type, Region.POLYGON)

    def test_polygon_size(self):
        """Test polygon size method"""
        points = [(0, 0), (100, 0), (100, 100)]
        polygon = Polygon.create(points)
        self.assertEqual(polygon.size(), 3)

    def test_polygon_get_point(self):
        """Test getting a point from polygon"""
        points = [(10.5, 20.5), (100.0, 50.0), (75.5, 150.5)]
        polygon = Polygon.create(points)
        
        for i, expected in enumerate(points):
            retrieved = polygon.get(i)
            self.assertAlmostEqual(retrieved[0], expected[0], places=1)
            self.assertAlmostEqual(retrieved[1], expected[1], places=1)

    def test_polygon_indexing(self):
        """Test polygon indexing with __getitem__"""
        points = [(10, 20), (100, 50), (75, 150)]
        polygon = Polygon.create(points)
        
        for i, expected in enumerate(points):
            retrieved = polygon[i]
            self.assertAlmostEqual(retrieved[0], expected[0], places=1)
            self.assertAlmostEqual(retrieved[1], expected[1], places=1)

    def test_polygon_invalid_index(self):
        """Test polygon with invalid index raises exception"""
        points = [(0, 0), (100, 0), (100, 100)]
        polygon = Polygon.create(points)
        
        with self.assertRaises(IndexError):
            polygon.get(-1)
        
        with self.assertRaises(IndexError):
            polygon.get(10)

    def test_polygon_iteration(self):
        """Test iterating over polygon points"""
        points = [(10, 20), (100, 50), (75, 150)]
        polygon = Polygon.create(points)
        
        retrieved_points = list(polygon)
        self.assertEqual(len(retrieved_points), len(points))
        
        for i, (retrieved, expected) in enumerate(zip(retrieved_points, points)):
            self.assertAlmostEqual(retrieved[0], expected[0], places=1)
            self.assertAlmostEqual(retrieved[1], expected[1], places=1)

    def test_polygon_minimum_points(self):
        """Test polygon creation requires minimum 3 points"""
        with self.assertRaises(AssertionError):
            Polygon.create([(0, 0), (100, 0)])
            
    def test_polygon_minimum_points_message(self):
        """Test polygon creation error message for insufficient points"""
        with self.assertRaisesRegex(AssertionError, "at least 3 points"):
            Polygon.create([(0, 0)])
            
    def test_polygon_one_point(self):
        """Test polygon creation with single point fails"""
        with self.assertRaisesRegex(AssertionError, "at least 3 points"):
            Polygon.create([(50, 100)])
            
    def test_polygon_requires_list(self):
        """Test polygon creation requires a list"""
        with self.assertRaisesRegex(AssertionError, "must be a list"):
            Polygon.create(((0, 0), (100, 0), (100, 100)))
            
    def test_polygon_requires_tuples(self):
        """Test polygon creation requires tuples for points"""
        with self.assertRaisesRegex(AssertionError, "must be tuples"):
            Polygon.create([[0, 0], [100, 0], [100, 100]])

    def test_polygon_string_representation(self):
        """Test polygon string representation"""
        points = [(0, 0), (100, 0), (100, 100), (0, 100)]
        polygon = Polygon.create(points)
        self.assertIn('Polygon', str(polygon))
        self.assertIn('4', str(polygon))


class TestPointRegion(unittest.TestCase):
    """Test Point region creation and properties"""

    def test_create_point_default(self):
        """Test creating a point with default values"""
        point = Point.create()
        self.assertIsNotNone(point)
        self.assertEqual(point.type, Region.POINT)

    def test_create_point_with_values(self):
        """Test creating a point with specific values"""
        point = Point.create(50.5, 100.5)
        self.assertIsNotNone(point)

    def test_point_get(self):
        """Test getting point coordinates"""
        x, y = 50.5, 100.5
        point = Point.create(x, y)
        retrieved_x, retrieved_y = point.get()
        self.assertAlmostEqual(retrieved_x, x, places=1)
        self.assertAlmostEqual(retrieved_y, y, places=1)

    def test_point_x_property(self):
        """Test point x coordinate property"""
        x, y = 50.5, 100.5
        point = Point.create(x, y)
        self.assertAlmostEqual(point.x, x, places=1)

    def test_point_y_property(self):
        """Test point y coordinate property"""
        x, y = 50.5, 100.5
        point = Point.create(x, y)
        self.assertAlmostEqual(point.y, y, places=1)

    def test_point_set(self):
        """Test setting point coordinates"""
        point = Point.create(10, 20)
        result = point.set(50, 100)
        self.assertIs(result, point)  # Test fluent interface
        
        retrieved_x, retrieved_y = point.get()
        self.assertAlmostEqual(retrieved_x, 50, places=1)
        self.assertAlmostEqual(retrieved_y, 100, places=1)
        
    def test_point_set_modifies_coordinates(self):
        """Test that set actually modifies the point coordinates"""
        point = Point.create(10, 20)
        point.set(100, 200)
        x, y = point.get()
        self.assertAlmostEqual(x, 100, places=1)
        self.assertAlmostEqual(y, 200, places=1)
        
    def test_point_set_multiple_times(self):
        """Test setting point coordinates multiple times"""
        point = Point.create(10, 20)
        point.set(50, 100)
        self.assertAlmostEqual(point.x, 50, places=1)
        self.assertAlmostEqual(point.y, 100, places=1)
        
        point.set(200, 300)
        self.assertAlmostEqual(point.x, 200, places=1)
        self.assertAlmostEqual(point.y, 300, places=1)
        
    def test_point_set_fluent_chaining(self):
        """Test fluent interface allows method chaining"""
        point = Point.create(0, 0)
        result = point.set(10, 20).set(30, 40)
        self.assertAlmostEqual(point.x, 30, places=1)
        self.assertAlmostEqual(point.y, 40, places=1)

    def test_point_string_representation(self):
        """Test point string representation"""
        point = Point.create(50, 100)
        self.assertIn('Point', str(point))
        self.assertIn('50', str(point))
        self.assertIn('100', str(point))
        
    def test_point_negative_coordinates(self):
        """Test point with negative coordinates"""
        point = Point.create(-50.5, -100.5)
        self.assertAlmostEqual(point.x, -50.5, places=1)
        self.assertAlmostEqual(point.y, -100.5, places=1)
        
    def test_point_zero_coordinates(self):
        """Test point with zero coordinates"""
        point = Point.create(0, 0)
        self.assertAlmostEqual(point.x, 0, places=1)
        self.assertAlmostEqual(point.y, 0, places=1)
        
    def test_point_large_coordinates(self):
        """Test point with large coordinates"""
        point = Point.create(10000.5, 20000.5)
        self.assertAlmostEqual(point.x, 10000.5, places=1)
        self.assertAlmostEqual(point.y, 20000.5, places=1)


class TestRegionTypeValidation(unittest.TestCase):
    """Test that region types are correctly distinguished"""
    
    def test_point_is_not_polygon(self):
        """Test that Point is a separate type from Polygon"""
        point = Point.create(50, 100)
        self.assertEqual(point.type, Region.POINT)
        self.assertNotEqual(point.type, Region.POLYGON)
        
    def test_polygon_is_not_point(self):
        """Test that Polygon with 3 points is not a Point"""
        polygon = Polygon.create([(0, 0), (100, 0), (50, 100)])
        self.assertEqual(polygon.type, Region.POLYGON)
        self.assertNotEqual(polygon.type, Region.POINT)
        
    def test_point_type_encoding(self):
        """Test Point type encoding is distinct"""
        point_code = Region.encode(Region.POINT)
        polygon_code = Region.encode(Region.POLYGON)
        self.assertNotEqual(point_code, polygon_code)
        self.assertEqual(point_code, 16)
        self.assertEqual(polygon_code, 4)


class TestCImplementationConsistency(unittest.TestCase):
    """Test consistency between Python wrapper and C implementation"""
    
    def test_point_roundtrip_c_implementation(self):
        """Test Point creation and retrieval through C implementation"""
        x_orig, y_orig = 123.456, 789.012
        point = Point.create(x_orig, y_orig)
        x_retrieved, y_retrieved = point.get()
        
        # C implementation should preserve float values
        self.assertAlmostEqual(x_retrieved, x_orig, places=2)
        self.assertAlmostEqual(y_retrieved, y_orig, places=2)
        
    def test_point_set_c_implementation(self):
        """Test Point.set() updates through C implementation"""
        point = Point.create(0, 0)
        x_new, y_new = 456.789, 321.654
        point.set(x_new, y_new)
        
        x_retrieved, y_retrieved = point.get()
        self.assertAlmostEqual(x_retrieved, x_new, places=2)
        self.assertAlmostEqual(y_retrieved, y_new, places=2)
        
    def test_polygon_minimum_enforced(self):
        """Test that C implementation doesn't accept < 3 points"""
        # This should fail at Python level before reaching C
        with self.assertRaises(AssertionError):
            Polygon.create([(0, 0), (100, 100)])

if __name__ == '__main__':
    unittest.main()
