import unittest
import io
import numpy as np
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from trax import MemoryImage, BufferImage


class TestMemoryImageUint8(unittest.TestCase):
    """Test MemoryImage with uint8 data (RGB)"""

    def setUp(self):
        """Set up test fixtures"""
        self.image_data = (np.random.random((100, 100, 3)) * 255).astype(np.uint8)
        self.memory_image = MemoryImage.create(self.image_data)

    def test_create_rgb_image(self):
        """Test creating a MemoryImage from RGB array"""
        self.assertIsNotNone(self.memory_image)

    def test_rgb_image_array_roundtrip(self):
        """Test that created image can be converted back to array"""
        retrieved_array = self.memory_image.array()
        self.assertIsNotNone(retrieved_array)

    def test_rgb_image_array_equality(self):
        """Test that retrieved array equals original"""
        retrieved_array = self.memory_image.array()
        np.testing.assert_array_equal(self.image_data, retrieved_array)

    def test_rgb_image_shape(self):
        """Test that image shape is preserved"""
        retrieved_array = self.memory_image.array()
        self.assertEqual(retrieved_array.shape, self.image_data.shape)

    def test_rgb_image_dtype(self):
        """Test that image dtype is preserved"""
        retrieved_array = self.memory_image.array()
        self.assertEqual(retrieved_array.dtype, self.image_data.dtype)


class TestMemoryImageUint16(unittest.TestCase):
    """Test MemoryImage with uint16 data (grayscale 16-bit)"""

    def setUp(self):
        """Set up test fixtures"""
        self.image_data = (np.random.random((100, 100, 1)) * 255 * 255).astype(np.uint16)
        self.memory_image = MemoryImage.create(self.image_data)

    def test_create_uint16_image(self):
        """Test creating a MemoryImage from uint16 array"""
        self.assertIsNotNone(self.memory_image)

    def test_uint16_image_array_roundtrip(self):
        """Test that created image can be converted back to array"""
        retrieved_array = self.memory_image.array()
        self.assertIsNotNone(retrieved_array)

    def test_uint16_image_array_equality(self):
        """Test that retrieved array equals original"""
        retrieved_array = self.memory_image.array()
        np.testing.assert_array_equal(self.image_data, retrieved_array)

    def test_uint16_image_shape(self):
        """Test that image shape is preserved"""
        retrieved_array = self.memory_image.array()
        self.assertEqual(retrieved_array.shape, self.image_data.shape)

    def test_uint16_image_dtype(self):
        """Test that image dtype is preserved"""
        retrieved_array = self.memory_image.array()
        self.assertEqual(retrieved_array.dtype, self.image_data.dtype)


class TestMemoryImageVariousSizes(unittest.TestCase):
    """Test MemoryImage with various image sizes"""

    def test_small_image(self):
        """Test with small image"""
        image_data = (np.random.random((10, 10, 3)) * 255).astype(np.uint8)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)

    def test_large_image(self):
        """Test with large image"""
        image_data = (np.random.random((1000, 1000, 3)) * 255).astype(np.uint8)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)

    def test_single_channel_image(self):
        """Test with single channel image"""
        image_data = (np.random.random((100, 100, 1)) * 255).astype(np.uint8)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)

class TestBufferImageJPEG(unittest.TestCase):
    """Test BufferImage with JPEG encoded data"""

    def setUp(self):
        """Set up test fixtures"""
        try:
            from PIL import Image
            self.pil_available = True
            self.original_array = (np.random.random((100, 100, 3)) * 255).astype(np.uint8)
            pil_image = Image.fromarray(self.original_array)
            buffer = io.BytesIO()
            pil_image.save(buffer, format='JPEG')
            self.jpeg_buffer = buffer.getvalue()
        except ImportError:
            self.pil_available = False
            self.skipTest("PIL not installed")

    def test_create_buffer_image_jpeg(self):
        """Test creating a BufferImage from JPEG buffer"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.jpeg_buffer)
        self.assertIsNotNone(buffer_image)

    def test_buffer_image_retrieve_buffer(self):
        """Test that buffer can be retrieved from BufferImage"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.jpeg_buffer)
        retrieved_buffer = buffer_image.buffer()
        self.assertIsNotNone(retrieved_buffer)

    def test_buffer_image_buffer_equality(self):
        """Test that retrieved buffer equals original"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.jpeg_buffer)
        retrieved_buffer = buffer_image.buffer()
        self.assertEqual(self.jpeg_buffer, retrieved_buffer)


class TestBufferImagePNG(unittest.TestCase):
    """Test BufferImage with PNG encoded data"""

    def setUp(self):
        """Set up test fixtures"""
        try:
            from PIL import Image
            self.pil_available = True
            self.original_array = (np.random.random((100, 100, 3)) * 255).astype(np.uint8)
            pil_image = Image.fromarray(self.original_array)
            buffer = io.BytesIO()
            pil_image.save(buffer, format='PNG')
            self.png_buffer = buffer.getvalue()
        except ImportError:
            self.pil_available = False
            self.skipTest("PIL not installed")

    def test_create_buffer_image_png(self):
        """Test creating a BufferImage from PNG buffer"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.png_buffer)
        self.assertIsNotNone(buffer_image)

    def test_buffer_image_png_retrieve_buffer(self):
        """Test that PNG buffer can be retrieved from BufferImage"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.png_buffer)
        retrieved_buffer = buffer_image.buffer()
        self.assertIsNotNone(retrieved_buffer)

    def test_buffer_image_png_buffer_equality(self):
        """Test that retrieved PNG buffer equals original"""
        if not self.pil_available:
            self.skipTest("PIL not installed")
        buffer_image = BufferImage.create(self.png_buffer)
        retrieved_buffer = buffer_image.buffer()
        self.assertEqual(self.png_buffer, retrieved_buffer)


class TestMemoryImageEdgeCases(unittest.TestCase):
    """Test MemoryImage edge cases"""

    def test_all_zeros_image(self):
        """Test with all-zeros image"""
        image_data = np.zeros((100, 100, 3), dtype=np.uint8)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)

    def test_all_max_values_image(self):
        """Test with all-max values image"""
        image_data = np.full((100, 100, 3), 255, dtype=np.uint8)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)

    def test_uint16_max_values(self):
        """Test uint16 image with max values"""
        image_data = np.full((100, 100, 1), 65535, dtype=np.uint16)
        memory_image = MemoryImage.create(image_data)
        retrieved = memory_image.array()
        np.testing.assert_array_equal(image_data, retrieved)


if __name__ == '__main__':
    unittest.main()



