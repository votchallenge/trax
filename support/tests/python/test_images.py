
import io
import numpy as np
from trax import MemoryImage, BufferImage
from PIL import Image

image1 = (np.random.random((100, 100, 3)) * 255).astype(np.uint8)

timage1 = MemoryImage.create(image1)

assert(np.array_equal(image1, timage1.array()))

image2 = (np.random.random((100, 100, 1)) * 255 * 255).astype(np.uint16)

timage2 = MemoryImage.create(image2)

assert(np.array_equal(image2, timage2.array()))


image3 = Image.fromarray(image1)
bimage = io.BytesIO()
image3.save(bimage, format='JPEG')
simage = bimage.getvalue()

timage3 = BufferImage.create(simage)

assert(simage == timage3.buffer())
