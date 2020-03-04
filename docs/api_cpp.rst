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

.. cpp:class:: Metadata

   A wrapper class for tracker metadata structure

   .. cpp:function:: Metadata(int region_formats, int image_formats, std::string tracker_name, std::string tracker_description, std::string tracker_family)

      Creates a new metadata object.

   .. cpp:function:: ~Metadata()

   .. cpp:function::  int image_formats()

      Returns supported image formats as a bit field.

   .. cpp:function::  int region_formats()

      Returns supported region formats as a bit field.

   .. cpp:function::  std::string tracker_name()

      Returns tracker name string or empty string.

   .. cpp:function::  std::string tracker_description()

      Returns tracker description string or empty string.

   .. cpp:function::  std::string tracker_family()

      Returns tracker family string or empty string.

.. cpp:class:: Logging

   A wrapper class for logging configuration structure

   .. cpp:function:: Logging(trax_logging logging)

   .. cpp:function:: Logging(trax_logger callback = NULL, void* data = NULL, int flags = 0)

   .. cpp:function:: ~Logging()

.. cpp:class:: Bounds

   .. cpp:function:: Bounds()

   .. cpp:function:: Bounds(trax_bounds bounds)


   .. cpp:function:: Bounds(float left, float top, float right, float bottom)

   .. cpp:function:: ~Bounds()

.. cpp:class:: Handle

   .. cpp:function:: ~Handle()

   .. cpp:function:: const Metadata metadata()

   .. cpp:function:: const bool terminate()

      Terminate session by sending quit message. Implicitly called when object is destroyed.

   .. cpp:function:: std::string get_error()

      Return last error string or empty string if no error has occured in last call to handle.

   .. cpp:function:: bool is_alive()

      Check if the handle is opened or not.


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

.. cpp:class:: Server

   .. cpp:function:: Server(Configuration configuration, Logging log)

      Sets up the protocol for the server side and returns a handle object.

   .. cpp:function:: ~Server()

   .. cpp:function:: int wait(Image& image, Region& region, Properties& properties)

      Waits for a valid protocol message from the client.

   .. cpp:function:: int reply(const Region& region, const Properties& properties)

      Sends a status reply to the client.

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

.. cpp:class:: Region

   .. cpp:function:: Region()

      Creates a new empty region.

   .. cpp:function:: Region(const Region& original)

      Creates a clone of region.

   .. cpp:function:: static Region create_special(int code)

      Creates a special region object. Only one paramter (region code) required.

   .. cpp:function:: static Region create_rectangle(float x, float y, float width, float height)

      Creates a rectangle region.

   .. cpp:function:: static Region create_polygon(int count)

      Creates a polygon region object for a given amout of points. Note that the coordinates of the points are arbitrary and have to be set after allocation.

   .. cpp:function:: static Region create_mask(int x, int y, int width, int height)

      Creates a mask region object of given size. Note that the mask data is not initialized.

   .. cpp:function:: ~Region()

      Releases region, frees allocated memory.

   .. cpp:function:: int type() const

      Returns type identifier of the region object.

   .. cpp:function:: bool empty() const

      Checks if region container is empty.

   .. cpp:function:: void set(int code)

      Sets the code of a special region.

   .. cpp:function:: int get() const

      Returns a code of a special region object.

   .. cpp:function:: void set(float x, float y, float width, float height)

      Sets the coordinates for a rectangle region.

   .. cpp:function:: void get(float* x, float* y, float* width, float* height) const

      Retreives coordinate from a rectangle region object.

   .. cpp:function:: void set_polygon_point(int index, float x, float y)

      Sets coordinates of a given point in the polygon.

   .. cpp:function:: void get_polygon_point(int index, float* x, float* y) const

      Retrieves the coordinates of a specific point in the polygon.

   .. cpp:function:: int get_polygon_count() const

      Returns the number of points in the polygon.

   .. cpp:function:: void get_mask_header(int* x, int* y, int* width, int* height) const

      Returns the header data of a mask region.
    
   .. cpp:function:: char* write_mask_row(int row)

      Returns a pointer for a writeable row in a data array of a mask.

   .. cpp:function:: const char* get_mask_row(int row) const

      Returns a read-only pointer for a row in a data array of a mask.

   .. cpp:function:: Bounds bounds() const

      Computes bounds of a region.

   .. cpp:function:: Region convert(int type) const

      Convert region to one of the other types if possible.

   .. cpp:function:: float overlap(const Region& region, const Bounds& bounds = Bounds()) const

      Calculates the Jaccard index overlap measure for the given regions with optional bounds that limit the calculation area.


.. cpp:class:: Properties

   .. cpp:function:: Properties()

      Create a property object.

   .. cpp:function:: Properties(const Properties& original)

      A copy constructor.

   .. cpp:function::  ~Properties()

      Destroy a properties object and clean up the memory.

   .. cpp:function:: int size()

      Return the number of elements.

   .. cpp:function:: void clear()

      Clear a properties object.

   .. cpp:function:: void set(const std::string key, const std::string value)

      Set a string property (the value string is cloned).

   .. cpp:function:: void set(const std::string key, int value)

      Set an integer property. The value will be encoded as a string.

   .. cpp:function:: void set(const std::string key, float value)

      Set an floating point value property. The value will be encoded as a string.

   .. cpp:function:: std::string get(const std::string key, const std::string& def)

      Get a string property.

   .. cpp:function:: int get(const std::string key, int def)

      Get an integer property. A stored string value is converted to an integer. If this is not possible or the property does not exist a given default value is returned.

   .. cpp:function:: float get(const std::string key, float def)

      Get an floating point value property. A stored string value is converted to an float. If this is not possible or the property does not exist a given default value is returned.

   .. cpp:function:: bool get(const std::string key, bool def)

      Get an boolean point value property. A stored string value is converted to an integer and checked if it is zero. If this is not possible or the property does not exist a given default value is returned.

   .. cpp:function:: void enumerate(Enumerator enumerator, void* object)

      Iterate over the property set using a callback function. An optional pointer can be given and is forwarded to the callback.

   .. cpp:function:: void from_map(const std::map<std::string, std::string>& m)

      Adds values from a dictionary to the properties object.

   .. cpp:function:: void to_map(std::map<std::string, std::string>& m)

      Copies key-value pairs in the properties object into the given dictionary.

   .. cpp:function:: void to_vector(std::vector<std::string>& v)

      Copies keys from the properties object into the given vector.


Integration example
-------------------

In C++ tracker implementations you can use either the C++ wrapper or basic C protocol implementation. The wrapper is more conveninent as it is object-oriented and provides automatic deallocation of resources via reference counting. Below is an sripped-down example of a C++ tracker skeleton with a typical tracking loop. Note that this is not a complete example and servers only as a demonstration of a typical tracker on a tracking-loop level.

.. code-block:: c++
  :linenos:

  #include <iostream>
  #include <fstream>

  using namescpace std;

  int main( int argc, char** argv)
  {
      int i;
      FILE* out;
      Rectangle region;
      Image image;
      Tracker tracker;

      ofstream out;
      output.open("trajectory.txt", ofstream::out);

      region = read_bounding_box();
      image = read_image(1);
      region = tracker.initialize(region, image);

      out << region << endl;

      for (i = 2; ; i++)
      {
        image = read_image(i);
        region = tracker.update(image);
        out << region << endl;
      }

      out.close();
      return 0;
  }

The code above can be modified to use the TraX protocol by including the C/C++ library header and changing the tracking loop to accept frames from the protocol insead of directly reading them from the filesystem. It also requires linking the protocol library (``libtrax``) when building the tracker executable.

.. code-block:: c++
  :linenos:

  #include <stdio.h>

  // Include TraX library header
  #include "trax.h"

  using namespace std;

  int main( int argc, char** argv)
  {
      int run = 1;

      // Initialize protocol
      trax::Server handle(trax::Metadata(TRAX_REGION_RECTANGLE,
                                  TRAX_IMAGE_PATH), trax_no_log);

      while(run)
      {
         trax::Image image;
         trax::Region region;
         trax::Properties properties;

         int tr = handle.wait(image, region, properties);

         // There are two important commands. The first one is
         // TRAX_INITIALIZE that tells the tracker how to initialize.
         if (tr == TRAX_INITIALIZE) {

            cv::Rect result = tracker.initialize(
                 trax::region_to_rect(region), trax::image_to_mat(image));

            handle.reply(trax::rect_to_region(result), trax::Properties());

         } else
         // The second one is TRAX_FRAME that tells the tracker what to process next.
         if (tr == TRAX_FRAME) {

            cv::Rect result = tracker.update(image_to_mat(image));
            handle.reply(trax::rect_to_region(result), trax::Properties());

         }
         // Any other command is either TRAX_QUIT or illegal, so we exit.
         else {
              run = 0;
         }

      }

      return 0;
  }



