OpenCV conversions library
==========================

`OpenCV <http://opencv.org/>`_ is one of most frequently used C++ libraries in computer vision. This support library provides conversion functions so that protocol image and region objects can be quickly converted to corresponding OpenCV objects and vice-versa.

Requirements and building
-------------------------

To compile the module you have to enable the ``BUILD_OPENCV`` flag in CMake build system (check out the :doc:`tutorial on compiling the project </tutorial_compiling>` for more details). You will of course need OpenCV library available on the system, version 2.4 or higher is supported.

Documentation
-------------

.. cpp:namespace:: trax

.. cpp:function:: cv::Mat image_to_mat(const Image& image)

   Converts a protocol image object to an OpenCV matrix that represents the image.

   :param image: Protocol image object
   :return: OpenCV matrix object

.. cpp:function:: cv::Rect region_to_rect(const Region& region)

   Converts a protocol region object to an OpenCV rectangle structure.

   :param image: Protocol region object
   :return: OpenCV rectangle structure

.. cpp:function:: std::vector<cv::Point2f> region_to_points(const Region& region)

   Converts a protocol region object to a list of OpenCV points.

   :param image: Protocol region object
   :return: List of points

.. cpp:function:: Image mat_to_image(const cv::Mat& mat)

   Converts an OpenCV matrix to a new protocol image object.

   :param mat: OpenCV image
   :return: Protocol image object

.. cpp:function:: Region rect_to_region(const cv::Rect rect)

   Converts an OpenCV rectangle structure to a protocol region object of type rectangle.

   :param rect: Rectangle structure
   :return: Protocol region object

.. cpp:function:: Region points_to_region(const std::vector<cv::Point2f> points)

   Converts a list of OpenCV points to a protocol region object of type polygon.

   :param rect: List of points
   :return: Protocol region object


.. cpp:function:: void draw_region(cv::Mat& canvas, const Region& region, cv::Scalar color, int width = 1)

   Draws a given region to an OpenCV image with a given color and line width.

   :param canvas: Target OpenCV image to which the region is drawn
   :param region: Protocol region object
   :param color: Color of the line as a an OpenCV scalar structure
   :param width: Width of the line
