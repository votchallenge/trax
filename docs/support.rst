Support modules
=================

Besides protocol implementation the repository also contains supporting libraries that help with some frequently needed functionalities.

Client utilities
----------------

The client support library provides a C++ client class that uses C++ protocol API to communicate with the tracker process, besides communication the class also takes care of launching tracker process, handling timeouts, logging, and other things. The class also supports setting up communication over streams as well as over TCP sockets.




CLI interface
~~~~~~~~~~~~~

Client support module also provides a simple cli interface to the client that can be used for simple tracker execution and protocol testing.


OpenCV conversions
------------------

`OpenCV <http://opencv.org/>`_ is one of most frequently used C++ libraries in computer vision. This support library provides conversion functions so that protocol image and region objects can be quickly converted to corresponding OpenCV objects and vice-versa.

.. cpp:namespace:: trax

.. cpp:function:: cv::Mat image_to_mat(const Image& image)

   Converts a protocol image object to an OpenCV matrix that represents the image.

   :param image: Protocol image object
   :return: OpenCV matrix object

.. cpp:function:: cv::Rect region_to_rect(const Region& region)

   Converts a protocol region object to an OpenCV rectangle structure.

   :param image: Protocol region object
   :return: OpenCV rectangle structure

.. cpp:function:: Image mat_to_image(const cv::Mat& mat)

   Converts an OpenCV matrix to a new protocol image object.

   :param mat: OpenCV image
   :return: Protocol image object

.. cpp:function:: Region rect_to_region(const cv::Rect rect)

   Converts an OpenCV rectangle structure to a protocol region object of type rectangle.

   :param rect: Rectangle structure
   :return: Protocol region object

.. cpp:function:: void draw_region(cv::Mat& canvas, const Region& region, cv::Scalar color, int width = 1)
 
   Draws a given region to an OpenCV image with a given color and line width.

   :param canvas: Target OpenCV image to which the region is drawn
   :param region: Protocol region object
   :param color: Color of the line as a an OpenCV scalar structure
   :param width: Width of the line
