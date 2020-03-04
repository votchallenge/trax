Reference C library
===================

The TraX protocol can be implemented using the protocol specification, the protocol is quite easy to implement in many high-level languages. However, a reference C implementation is provided to serve as a practical model of implementation and to make the adoption of the protocol easier.

Requirements and building
-------------------------

The library is built using `CMake <https://cmake.org/>`_ build tool which generates build instructions for a given platform. The code is written in C89 for maximum portability and the library has no external dependencies apart from the standard C library. For more details on building check out the :doc:`tutorial on compiling the project </tutorial_compiling>`.

Documentation
-------------

All the public functionality of the library is described in the ``trax.h`` header file, below is a summary of individual functions that are available in the library.

Communication
~~~~~~~~~~~~~

.. c:macro:: TRAX_ERROR

    Value that indicates protocol error

.. c:macro:: TRAX_OK

    Value that indicates success of a function call

.. c:macro:: TRAX_HELLO

    Value that indicates introduction message

.. c:macro:: TRAX_INITIALIZE

    Value that indicates initialization message

.. c:macro:: TRAX_FRAME

    Value that indicates frame message

.. c:macro:: TRAX_QUIT

    Value that indicates quit message

.. c:macro:: TRAX_STATE

    Value that indicates status message

.. c:type:: trax_logging

    Structure that describes logging handle.

.. c:type:: trax_bounds

    Structure that describes region bounds.

.. c:type:: trax_handle

    Structure that describes a protocol state for either client or server.

.. c:type:: trax_image

    Structure that describes an image.

.. c:type:: trax_region

    Structure that describes a region.

.. c:type:: trax_properties

    Structure that contains an key-value dictionary.

.. c:var:: trax_logging trax_no_log

    A constant to indicate that no logging will be done.

.. c:var:: trax_bounds trax_no_bounds

    A constant to indicate that here are no bounds.

.. :c:function:: void(*trax_logger)(const char *string, int length, void *obj)

   A logger callback function type. Functions with this signature can be used for logging protocol data. Everytime a function is called it is given a character buffer of a specified length that has to be handled by the logger. The optional pointer to additional data may be passed to the callback to access additional data.

.. c:function:: const char* trax_version()

   Returns a string version of the library for debugging purposes. If possible, this version is defined during compilation time and corresponds to Git hash for the current revision.

   :return: Version string as a constant character array

.. c:function:: trax_metadata* trax_metadata_create(int region_formats, int image_formats, int channels, const char* tracker_name, const char* tracker_description, const char* tracker_family)

   :param region_formats: Supported regions formats bit-set.
   :param image_formats: Supported image formats bit-set.
   :param channels: Required image channels bit-set. If empty then only `TRAX_CHANNEL_VISIBLE` is assumed.

   Create a tracker metadata structure returning its pointer

   :return: A pointer to a metadata structure that can be released using :c:func:`trax_metadata_release`.

.. c:function:: void trax_metadata_release(trax_metadata** metadata)

   Releases a given metadata structure, clearing its memory.

   :param metadata: Pointer of a pointer of tracker metadata structure.

.. c:function:: trax_logging trax_logger_setup(trax_logger callback, void* data, int flags)

   A function that can be used to initialize a logging configuration structure.

   :param callback: Callback function used to process a chunk of log data
   :param data: Additional data passed to the callback function as an argument
   :param flags: Optional flags for logger
   :return: A logging structure for the given data

.. c:function:: trax_logging trax_logger_setup_file(FILE* file)

   A handy function to initialize a logging configuration structure for file logging. Internally the function calls :c:func:`trax_logger_setup`.

   :param file: File object, opened for writing, can also be ``stdout`` or ``stderr``
   :return: A logging structure for the given file

.. c:function:: trax_handle* trax_client_setup_file(int input, int output, trax_logging log)

   Setups the protocol state object for the client. It is assumed that the tracker process is already running (how this is done is not specified by the protocol). This function tries to parse tracker's introduction message and fails if it is unable to do so or if the handshake fails (e.g. unsupported format version).

   :param input: Stream identifier, opened for reading, used to read server output
   :param output: Stream identifier, opened for writing, used to write messages
   :param log: Logging structure
   :return: A handle object used for further communication or ``NULL`` if initialization was unsuccessful

.. c:function:: trax_handle* trax_client_setup_socket(int server, int timeout, trax_logging log)

   Setups the protocol state object for the client using a bi-directional socket. It is assumed that the connection was already established (how this is done is not specified by the protocol). This function tries to parse tracker's introduction message and fails if it is unable to do so or if the handshake fails (e.g. unsupported format version).

   :param server: Socket identifier, used to read communcate with tracker
   :param log: Logging structure
   :return: A handle object used for further communication or ``NULL`` if initialization was unsuccessful

.. c:function:: int trax_client_wait(trax_handle* client, trax_region** region, trax_properties* properties)

   Waits for a valid protocol message from the server.

   :param client: Client state object
   :param region: Pointer to current region for an object, set if the response is :c:macro:`TRAX_STATE`, otherwise ``NULL``
   :param properties: Additional properties
   :return: Integer value indicating status, can be either :c:macro:`TRAX_STATE`, :c:macro:`TRAX_QUIT`, or :c:macro:`TRAX_ERROR`

.. c:function:: int trax_client_initialize(trax_handle* client, trax_image* image, trax_region* region, trax_properties* properties)

    Sends an initialize message to server.

   :param client: Client state object
   :param image: Image frame data
   :param region: Initialization region
   :param properties: Additional properties object
   :return: Integer value indicating status, can be either :c:macro:`TRAX_OK` or :c:macro:`TRAX_ERROR`

.. c:function:: int trax_client_frame(trax_handle* client, trax_image* image, trax_properties* properties)

    Sends a frame message to server.

   :param client: Client state object
   :param image: Image frame data
   :param properties: Additional properties
   :return: Integer value indicating status, can be either :c:macro:`TRAX_OK` or :c:macro:`TRAX_ERROR`

.. c:function:: trax_handle* trax_server_setup(trax_metadata* metadata, trax_logging log)

   Setups the protocol for the server side and returns a handle object.

   :param metadata: Tracker metadata
   :param log: Logging structure
   :return: A handle object used for further communication or ``NULL`` if initialization was unsuccessful

.. c:function:: trax_handle* trax_server_setup_file(trax_metadata* metadata, int input, int output, trax_logging log)

   Setups the protocol for the server side based on input and output stream and returns a handle object.

   :param metadata: Tracker metadata
   :param input: Stream identifier, opened for reading, used to read client output
   :param output: Stream identifier, opened for writing, used to write messages
   :param log: Logging structure
   :return: A handle object used for further communication or ``NULL`` if initialization was unsuccessful

.. c:function:: int trax_server_wait(trax_handle* server, trax_image** image, trax_region** region, trax_properties* properties)

    Waits for a valid protocol message from the client.

   :param server: Server state object
   :param image: Pointer to image frame data, set if the response is not :c:macro:`TRAX_QUIT` or :c:macro:`TRAX_ERROR`, otherwise ``NULL``
   :param region: Pointer to initialization region, set if the response is :c:macro:`TRAX_INITIALIZE`, otherwise ``NULL``
   :param properties: Additional properties
   :return: Integer value indicating status, can be either :c:macro:`TRAX_INITIALIZE`, :c:macro:`TRAX_FRAME`, :c:macro:`TRAX_QUIT`, or :c:macro:`TRAX_ERROR`

.. c:function:: int trax_server_reply(trax_handle* server, trax_region* region, trax_properties* properties)

    Sends a status reply to the client.

   :param server: Server state object
   :param region: Current region of an object
   :param properties: Additional properties
   :return: Integer value indicating status, can be either :c:macro:`TRAX_OK` or :c:macro:`TRAX_ERROR`

.. c:function:: int trax_terminate(trax_handle* handle)

   Used in client and server. Closes communication, sends quit message if needed. This function is implicitly
   called in :c:func:`trax_cleanup`.

   :param handle: Server or client state object
   :return: Integer value indicating status, can be either :c:macro:`TRAX_OK` or :c:macro:`TRAX_ERROR`

.. c:function:: int trax_cleanup(trax_handle** handle)

   Used in client and server. Closes communication, sends quit message if needed. Releases the handle structure.

   :param handle: Pointer to state object pointer
   :return: Integer value indicating status, can be either :c:macro:`TRAX_OK` or :c:macro:`TRAX_ERROR`

.. c:function:: const char* trax_get_error(trax_handle* handle)

   Retrieve last error message encountered by the server or client. 

   :param handle: Pointer to state object
   :return: Returns NULL if no error occured.

.. c:function:: int trax_is_alive(trax_handle* handle)

   Check if the handle has been terminated.

   :param handle: Pointer to state object
   :return: Returns zero if handle is not alive and non-zero if it is.

.. c:function:: int trax_set_parameter(trax_handle* handle, int id, int value)

   Sets the parameter of the client or server instance.

.. c:function:: int trax_get_parameter(trax_handle* handle, int id, int* value)

   Gets the parameter of the client or server instance.


ImageList
~~~~~~~~~

.. c:macro::  TRAX_CHANNEL_VISIBLE

    Visible light channel identifier.

.. c:macro::  TRAX_CHANNEL_DEPTH

    Depth channel identifier.

.. c:macro::  TRAX_CHANNEL_IR

    IR channel identifier.

.. c:macro::  TRAX_CHANNELS

    Number of available channels.

.. c:macro::  TRAX_CHANNEL_INDEX(I)

    Convert channel identifier into index.

.. c:macro::  TRAX_CHANNEL_ID(I)

    Convert channel index into identifier.

.. c:function:: trax_image_list* trax_image_list_create()

    Create an emptry image list

   :returns: Pointer to image list container

.. c:function:: void trax_image_list_release(trax_image_list** list)

    Release image list structure, does not release any channel images

   :param list: Image list container pointer

.. c:function:: void trax_image_list_clear(trax_image_list* list)

    Cleans image list, releases all allocated channel images

   :param list: Image list container pointer

.. c:function:: trax_image* trax_image_list_get(const trax_image_list* list, int channel)

    Get image for a specific channel

   :param list: Image list structure pointer
   :param channel: Channel idenfifier
   :returns: Image structure pointer

.. c:function:: void trax_image_list_set(trax_image_list* list, trax_image* image, int channel)

    Set image for a specific channel

   :param list: Image list structure pointer
   :param image: Image structure pointer
   :param channel: Channel idenfifier
   :returns: Pointer to null-terminated character array

.. c:function:: int trax_image_list_count(int channels)

     Count available channels in provided bit-set

   :param channels: Bit-set of channel identifiers
   :returns: Number of on bits.

Image
~~~~~

.. c:macro::  TRAX_IMAGE_EMPTY

    Empty image type, not usable in any way but to signify that there is no data.

.. c:macro::  TRAX_IMAGE_PATH

    Image data is provided in a file on a file system. Only a path is provided.

.. c:macro::  TRAX_IMAGE_URL

    Image data is provided in a local or remote resource. Only a URL is provided.

.. c:macro::  TRAX_IMAGE_MEMORY

    Image data is provided in a memory buffer and can be accessed directly.

.. c:macro::  TRAX_IMAGE_BUFFER

    Image data is provided in a memory buffer but has to be decoded first.

.. c:macro::  TRAX_IMAGE_BUFFER_ILLEGAL

    Image buffer is of an unknown data type.

.. c:macro::  TRAX_IMAGE_BUFFER_PNG

    Image data is encoded as PNG image.

.. c:macro::  TRAX_IMAGE_BUFFER_JPEG

    Image data is encoded as JPEG image.

.. c:macro::  TRAX_IMAGE_MEMORY_ILLEGAL

    Image data is available in an unknown format.

.. c:macro::  TRAX_IMAGE_MEMORY_GRAY8

    Image data is available in 8 bit per pixel format.

.. c:macro::  TRAX_IMAGE_MEMORY_GRAY16

    Image data is available in 16 bit per pixel format.

.. c:macro::  TRAX_IMAGE_MEMORY_RGB

    Image data is available in RGB format with three bytes per pixel.

.. c:function:: void trax_image_release(trax_image** image)

   Releases image structure, frees allocated memory.

   :param image: Pointer to image structure pointer (the pointer is set to ``NULL`` if the structure is destroyed successfuly)

.. c:function:: trax_image* trax_image_create_path(const char* path)

   Creates a file-system path image description.

   :param url: File path string, it is copied internally
   :returns: Image structure pointer

.. c:function:: trax_image* trax_image_create_url(const char* url)

   Creates a URL path image description.

   :param url: URL string, it is copied internally
   :returns: Image structure pointer

.. c:function:: trax_image* trax_image_create_memory(int width, int height, int format)

   Creates a raw in-memory buffer image description. The memory is not initialized, you have do this manually.

   :param width: Image width
   :param height: Image height
   :param format: Image format, see format type constants for options
   :returns: Image structure pointer

.. c:function:: trax_image* trax_image_create_buffer(int length, const char* data)

   Creates a file buffer image description.

   :param length: Length of the buffer
   :param data: Character array with data, the buffer is copied
   :returns: Image structure pointer

.. c:function:: int trax_image_get_type(const trax_image* image)

   Returns a type of the image handle.

   :param image: Image structure pointer
   :returns: Image type code, see image type constants for more details

.. c:function:: const char* trax_image_get_path(const trax_image* image)

   Returns a file path from a file-system path image description. This function returns a pointer to the internal data which should not be modified.

   :param image: Image structure pointer
   :returns: Pointer to null-terminated character array

.. c:function:: const char* trax_image_get_url(const trax_image* image)

   Returns a file path from a URL path image description. This function returns a pointer to the internal data which should not be modified.

   :param image: Image structure pointer
   :returns: Pointer to null-terminated character array

.. c:function:: void trax_image_get_memory_header(const trax_image* image, int* width, int* height, int* format)

   Returns the header data of a memory image.

   :param image: Image structure pointer
   :param width: Pointer to variable that is populated with width of the image
   :param height: Pointer to variable that is populated with height of the image
   :param format: Pointer to variable that is populated with format of the image, see format constants for options

.. c:function:: char* trax_image_write_memory_row(trax_image* image, int row)

   Returns a pointer for a writeable row in a data array of an image.

   :param image: Image structure pointer
   :param row: Number of row
   :returns: Pointer to character array of the line

.. c:function:: const char* trax_image_get_memory_row(const trax_image* image, int row)

   Returns a read-only pointer for a row in a data array of an image.

   :param image: Image structure pointer
   :param row: Number of row
   :returns: Pointer to character array of the line

.. c:function:: const char* trax_image_get_buffer(const trax_image* image, int* length, int* format)

   Returns a file buffer and its length. This function returns a pointer to the internal data which should not be modified.

   :param image: Image structure pointer
   :param length: Pointer to variable that is populated with buffer length
   :param format: Pointer to variable that is populated with buffer format code
   :returns: Pointer to character array


Region
~~~~~~

.. c:macro::  TRAX_REGION_EMPTY

    Empty region type, not usable in any way but to signify that there is no data.

.. c:macro::  TRAX_REGION_SPECIAL

    Special code region type, only one value avalable that can have a defined meaning.

.. c:macro::  TRAX_REGION_RECTANGLE

    Rectangle region type. Left, top, width and height values available.

.. c:macro::  TRAX_REGION_POLYGON

    Polygon region type. Three or more points available with x and y coordinates.

... c:macro::  TRAX_REGION_MASK

..    Mask region type. A per-pixel binary mask.

.. c:macro::  TRAX_REGION_ANY

    Any region type, a shortcut to specify that any supported region type can be used.

.. c:function:: void trax_region_release(trax_region** region)

   Releases region structure, frees allocated memory.

   :param region: Pointer to region structure pointer (the pointer is set to ``NULL`` if the structure is destroyed successfuly)

.. c:function:: int trax_region_get_type(const trax_region* region)

   Returns type identifier of the region object.

   :param region: Region structure pointer
   :returns: One of the region type constants

.. c:function:: trax_region* trax_region_create_special(int code)

   Creates a special region object.

   :param code: A numerical value that is contained in the region type
   :returns: A pointer to the region object

.. c:function:: void trax_region_set_special(trax_region* region, int code)

   Sets the code of a special region.

   :param region: Region structure pointer
   :param code: The new numerical value

.. c:function:: int trax_region_get_special(const trax_region* region)

   Returns a code of a special region object if the region is of *special* type.

   :param region: Region structure pointer
   :returns: The numerical value

.. c:function:: trax_region* trax_region_create_rectangle(float x, float y, float width, float height)

   Creates a rectangle region.

   :param x: Left offset
   :param y: Top offset
   :param width: Width of rectangle
   :param height: Height of rectangle
   :returns: A pointer to the region object

.. c:function:: void trax_region_set_rectangle(trax_region* region, float x, float y, float width, float height)

   Sets the coordinates for a rectangle region.

   :param region: A pointer to the region object
   :param x: Left offset
   :param y: Top offset
   :param width: Width of rectangle
   :param height: Height of rectangle

.. c:function:: void trax_region_get_rectangle(const trax_region* region, float* x, float* y, float* width, float* height)

   Retreives coordinate from a rectangle region object.

   :param region: A pointer to the region object
   :param x: Pointer to left offset value variable
   :param y: Pointer to top offset value variable
   :param width: Pointer to width value variable
   :param height: Pointer to height value variable

.. c:function:: trax_region* trax_region_create_polygon(int count)

   Creates a polygon region object for a given amout of points. Note that the coordinates of the points are arbitrary and have to be set after allocation.

   :param code: The number of points in the polygon
   :returns: A pointer to the region object

.. c:function:: void trax_region_set_polygon_point(trax_region* region, int index, float x, float y)

   Sets coordinates of a given point in the polygon.

   :param region: A pointer to the region object
   :param index: Index of point
   :param x: Horizontal coordinate
   :param y: Vertical coordinate

.. c:function:: void trax_region_get_polygon_point(const trax_region* region, int index, float* x, float* y)

   Retrieves the coordinates of a specific point in the polygon.

   :param region: A pointer to the region object
   :param index: Index of point
   :param x: Pointer to horizontal coordinate value variable
   :param y: Pointer to vertical coordinate value variable

.. c:function:: int trax_region_get_polygon_count(const trax_region* region)

   Returns the number of points in the polygon.

   :param region: A pointer to the region object
   :return: Number of points

.. c:function:: trax_region* trax_region_create_mask(int x, int y, int width, int height)

   Creates a mask region object of given size. Note that the mask data is not initialized.

   :param x: Left offset 
   :param y: Top offset
   :param width: Mask width
   :param height: Mask height
   :return: A pointer to the region object

.. c:function:: void trax_region_get_mask_header(const trax_region* region, int* x, int* y, int* width, int* height)

   Returns the header data of a mask region.

   :param region: A pointer to the region object
   :param x: Pointer to left offset value variable
   :param y: Pointer to top offset value variable
   :param width: Pointer to width value variable
   :param height: Pointer to height value variable

.. c:function:: char* trax_region_write_mask_row(trax_region* region, int row)

   Returns a pointer for a writeable row in a data array of a mask.

   :param region: Region structure pointer
   :param row: Number of row
   :returns: Pointer to character array of the line

.. c:function:: const char* trax_region_get_mask_row(const trax_region* region, int row)

   Returns a read-only pointer for a row in a data array of a mask.

   :param region: Region structure pointer
   :param row: Number of row
   :returns: Pointer to character array of the line

.. c:function:: trax_bounds trax_region_bounds(const trax_region* region)

   Calculates a bounding box region that bounds the input region.

   :param region: A pointer to the region object
   :return: A bounding box structure that contains values for left, top, right, and bottom

.. c:function:: trax_region* trax_region_clone(const trax_region* region)

   Clones a region object.

   :param region: A pointer to the region object
   :return: A cloned region object pointer

.. c:function:: trax_region* trax_region_convert(const trax_region* region, int format)

   Converts region between different formats (if possible).

   :param region: A pointer to the region object
   :param format: One of the format type constants
   :return: A converted region object pointer

.. c:function:: float trax_region_contains(const trax_region* region, float x, float y)

   Calculates if the region contains a given point.

   :param region: A pointer to the region object
   :param x: X coordinate of the point
   :param y: Y coordinate of the point
   :return: Returns zero if the point is not in the region or one if it is

.. c:function:: float trax_region_overlap(const trax_region* a, const trax_region* b, const trax_bounds bounds)

   Calculates the spatial Jaccard index for two regions (overlap).

   :param a: A pointer to the region object
   :param b: A pointer to the region object
   :return: A bounds structure to contain only overlap within bounds or :c:data:`trax_no_bounds` if no bounds are specified

.. c:function:: char* trax_region_encode(const trax_region* region)

   Encodes a region object to a string representation.

   :param region: A pointer to the region object
   :return: A character array with textual representation of the region data

.. c:function:: trax_region* trax_region_decode(const char* data)

   Decodes string representation of a region to an object.

   :param region: A character array with textual representation of the region data
   :return: A pointer to the region object or ``NULL`` if string does not contain valid region data


Properties
~~~~~~~~~~

.. c:function:: trax_properties* trax_properties_create()

   Create an empty properties dictionary.

   :returns: A pointer to a properties object

.. c:function:: void trax_properties_release(trax_properties** properties)

   Destroy a properties object and clean up the memory.

   :param properties: A pointer to a properties object pointer

.. c:function:: void trax_properties_clear(trax_properties* properties)

   Clears a properties dictionary making it empty.

   :param properties: A pointer to a properties object

.. c:function:: void trax_properties_set(trax_properties* properties, const char* key, const char* value)

   Set a string property for a given key. The value string is cloned.

   :param properties: A pointer to a properties object
   :param key: A key for the property, only keys valid according to the protocol are accepted
   :param value: The value for the property, the string is cloned internally

.. c:function:: void trax_properties_set_int(trax_properties* properties, const char* key, int value)

   Set an integer property. The value will be encoded as a string.

   :param properties: A pointer to a properties object
   :param key: A key for the property, only keys valid according to the protocol are accepted
   :param value: The value for the property

.. c:function:: void trax_properties_set_float(trax_properties* properties, const char* key, float value)

   Set an floating point value property. The value will be encoded as a string.

   :param properties: A pointer to a properties object
   :param key: A key for the property, only keys valid according to the protocol are accepted
   :param value: The value for the property

.. c:function:: char* trax_properties_get(const trax_properties* properties, const char* key)

   Get a string property. The resulting string is a clone of the one stored so it should be released when not needed anymore.

   :param properties: A pointer to a properties object
   :param key: A key for the property
   :returns: The value for the property or ``NULL`` if there is no value associated with the key

.. c:function:: int trax_properties_get_int(const trax_properties* properties, const char* key, int def)

    Get an integer property. A stored string value is converted to an integer. If this is not possible or the property does not exist a given default value is returned.

   :param properties: A pointer to a properties object
   :param key: A key for the property
   :param def: Default value for the property
   :returns: The value for the property or default value if there is no value associated with the key or conversion from string is impossible

.. c:function:: float trax_properties_get_float(const trax_properties* properties, const char* key, float def)

   Get an floating point value property. A stored string value is converted to an integer. If this is not possible or the property does not exist a given default value is returned.

   :param properties: A pointer to a properties object
   :param key: A key for the property
   :param def: Default value for the property
   :returns: The value for the property or default value if there is no value associated with the key or conversion from string is impossible

.. c:function:: int trax_properties_count(const trax_properties* properties)

   Get a number of all pairs in the properties object.

   :param properties: A pointer to a properties object
   :returns: Number of key-value pairs in the properties object


.. :c:function:: void(*trax_enumerator)(const char *key, const char *value, const void *obj)


.. c:function:: void trax_properties_enumerate(trax_properties* properties, trax_enumerator enumerator, const void* object)

   Iterate over the property set using a callback function. An optional pointer can be given and is forwarded to the callback.

   :param properties: A pointer to a properties object
   :param enumerator: A pointer to the enumerator function that is called for every key-value pair
   :param object: A pointer to additional data for the enumerator function

Integration example
-------------------

The library can be easily integrated into C and C++ code (although a C++ wrapper also exists) and can be also linked into other programming languages that enable linking of C libraries. Below is an sripped-down example of a C tracker skeleton with a typical tracking loop. Note that this is not a complete example and servers only as a demonstration of a typical tracker on a tracking-loop level.

.. code-block:: c
  :linenos:

  #include <stdio.h>

  int main( int argc, char** argv)
  {
      int i;
      FILE* out;
      rectangle_type region;
      image_type image;

      out = fopen("trajectory.txt", "w");

      region = read_bounding_box();
      image = read_image(1);
      region = initialize_tracker(region, image);

      write_frame(out, region);

      for (i = 2; ; i++)
      {
        image = read_image(i);
        region = update_tracker(image);
        write_frame(out, region);
      }

      fclose(out);
      return 0;
  }

The code above can be modified to use the TraX protocol by including the C library header and changing the tracking loop to accept frames from the protocol insead of directly reading them from the filesystem. It also requires linking the protocol library (``libtrax``) when building the tracker executable.

.. code-block:: c
  :linenos:

  #include <stdio.h>

  // Include TraX library header
  #include "trax.h"

  int main( int argc, char** argv)
  {
      int run = 1;
      trax_image_list* img = NULL;
      trax_region* reg = NULL;

      // Call trax_server_setup at the beginning
      trax_handle* handle;
      trax_metadata* meta = trax_metadata_create(TRAX_REGION_RECTANGLE, TRAX_IMAGE_PATH, TRAX_CHANNEL_VISIBLE, "Name", NULL, NULL);

      handle = trax_server_setup(meta, trax_no_log);

      trax_metadata_release(&meta);

      while(run)
      {
          int tr = trax_server_wait(handle, &img, &reg, NULL);

          // There are two important commands. The first one is
          // TRAX_INITIALIZE that tells the tracker how to initialize.
          if (tr == TRAX_INITIALIZE) {

              rectangle_type region = initialize_tracker(
                  region_to_rectangle(reg), load_image(img));
              trax_server_reply(handle, rectangle_to_region(region), NULL);

          } else
          // The second one is TRAX_FRAME that tells the tracker what to process next.
          if (tr == TRAX_FRAME) {

              rectangle_type region = update_tracker(load_image(trax_image_list_get(img, TRAX_CHANNEL_VISIBLE)));
              trax_server_reply(handle, rectangle_to_region(region), NULL);

          }
          // Any other command is either TRAX_QUIT or illegal, so we exit.
          else {
              run = 0;
          }

          if (img) {
              trax_image_list_clear(img); // Also delete individual images
              trax_image_list_release(&img);
          }
          if (reg) trax_region_release(&reg);

      }

      // TraX: Call trax_cleanup at the end
      trax_cleanup(&handle);

      return 0;
  }

