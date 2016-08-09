Library C++ wrapper
===================

The main functionality of the reference library is written in pure C, however, it also offers a C++ wrapper if used with a C++ compiler. This wrapper uses classes and objects as well as reference counting for memory management, making is more suitable choice when using the reference library in a C++ algorithm.

Requirements and building
-------------------------

No additional requirements are necessary for building the wrapper but a C++ compiler.

Documentation
-------------

The wrapper is composed of several classes, mostly following the underlying C functions. All the classes are contained in ``trax`` namespace.

.. cpp:namespace:: trax

.. cpp:class:: Configuration

A wrapper class for 

.. cpp:function:: Configuration(trax_configuration config)


.. cpp:function:: Configuration(int image_formats, int region_formats)


.. cpp:function:: ~Configuration()

.. cpp:class:: Logging 

A wrapper class for 

.. cpp:function:: Logging(trax_logging logging)

.. cpp:function:: Logging(trax_logger callback = NULL, void* data = NULL, int flags = 0)

.. cpp:function:: ~Logging()

.. cpp:class:: Bounds 

.. cpp:function:: Bounds()

.. cpp:function:: Bounds(trax_bounds bounds)


.. cpp:function:: Bounds(float left, float top, float right, float bottom)

.. cpp:function:: ~Bounds()


.. cpp:class:: Client

.. cpp:function:: Client(int input, int output, Logging logger)

   Sets up the protocol for the client side and returns a handle object.

.. cpp:function:: Client(int server, Logging logger,  int timeout = -1)

   Sets up the protocol for the client side and returns a handle object.

.. cpp:function:: ~Client()

.. cpp:function:: int wait(Region& region, Properties& properties)

   Waits for a valid protocol message from the server.

.. cpp:function:: int initialize(const Image& image, const Region& region, const Properties& properties)

   Sends an initialize message.

.. cpp:function:: int frame(const Image& image, const Properties& properties)

   Sends a frame message.

.. cpp:function:: const Configuration configuration()


.. cpp:class:: Server


.. cpp:function:: Server(Configuration configuration, Logging log)

   Sets up the protocol for the server side and returns a handle object.

.. cpp:function:: ~Server()

.. cpp:function:: int wait(Image& image, Region& region, Properties& properties)

   Waits for a valid protocol message from the client.

.. cpp:function:: int reply(const Region& region, const Properties& properties)

   Sends a status reply to the client.

.. cpp:function:: const Configuration configuration()


.. cpp:class:: Image

.. cpp:function:: Image()

.. cpp:function:: Image(const Image& original)

.. cpp:function:: static Image create_path(const std::string& path)

   Creates a file-system path image description. See :c:func:`trax_image_create_path`.

.. cpp:function:: static Image create_url(const std::string& url)

   Creates a URL path image description.  See :c:func:`trax_image_create_url`.

.. cpp:function:: static Image create_memory(int width, int height, int format)

   Creates a raw buffer image description.See :c:func:`trax_image_create_memory`.

.. cpp:function:: static Image create_buffer(int length, const char* data)

   Creates a file buffer image description. See :c:func:`trax_image_create_buffer`.

.. cpp:function::  ~Image()

   Releases image structure, frees allocated memory.
 
.. cpp:function:: int type() const

   Returns a type of the image handle. See :c:func:`trax_image_get_type`.

.. cpp:function:: bool empty() const

   Checks if image container is empty.

.. cpp:function:: const std::string get_path() const

   Returns a file path from a file-system path image description. This function returns a pointer to the internal data which should not be modified.

.. cpp:function:: const std::string get_url() const

   Returns a file path from a URL path image description. This function returns a pointer to the internal data which should not be modified.

.. cpp:function:: void get_memory_header(int* width, int* height, int* format) const

   Returns the header data of a memory image.

.. cpp:function:: char* write_memory_row(int row)

   Returns a pointer for a writeable row in a data array of an image.

.. cpp:function:: const char* get_memory_row(int row) const

   Returns a read-only pointer for a row in a data array of an image.

.. cpp:function:: const char* get_buffer(int* length, int* format) const

   Returns a file buffer and its length. This function returns a pointer to the internal data which should not be modified.


Integration tutorial
--------------------



