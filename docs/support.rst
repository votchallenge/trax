Support modules
=================

Besides protocol implementation the repository also contains supporting libraries that help with some frequently needed functionalities.

Client utilities
----------------

The client support library provides a C++ client class that uses C++ protocol API to communicate with the tracker process, besides communication the class also takes care of launching tracker process, handling timeouts, logging, and other things. The class also supports setting up communication over streams as well as over TCP sockets. A detailed tutorial on how to use the client library is available :doc:`here </tutorial_client>`. To compile the module you have to enable ``BUILD_CLIENT`` flag in CMake build system (check out the :doc:`tutorial on compiling the project </tutorial_compiling>` for more details).

.. cpp:namespace:: trax

.. cpp:function:: int load_trajectory(const std::string& file, std::vector<Region>& trajectory)

   Utility function to load a trajectory (a sequence of object states) form a text file.

   :param file: Filename string
   :param trajectory: Empty vector that will be populated with region states
   :return: Number of read states

.. cpp:function:: void save_trajectory(const std::string& file, std::vector<Region>& trajectory)

   Utility function to save a trajectory (a sequence of object states) to a text file.

   :param file: Filename string
   :param trajectory: Vector that contains the trajectory

CLI interface
~~~~~~~~~~~~~

Client support module also provides a `traxclient`, a simple CLI (command line interface) to the client that can be used for simple tracker execution as well as a `traxtest` utility that can be used for protocol support testing.

If the OpenCV support module (below) is also compiled then the CLI interface uses it for some extra conversions that are otherwise not supported (e.g. loading images and sending them in their raw form over the communication channel if the server requests it) and a `traxplayer` utility that can be used to interactively test trackers with the data from video or camera stream.

The `traxclient` utility is in fact quite powerful and can be used to perform many batch experiments if run multiple times with different parameters. It is internally used by the `VOT toolkit <https://github.com/votchallenge/vot-toolkit>`_ and can be used to perform other batch experiments as well. If we assume that image sequence is stored in `images.txt` file (one absolute image path per line) and that groundtruth are stored in `groundtruth.txt` file (one region per line, corresponding to the images), we can run a tracker on this sequence by calling::

    traxclient -g groundtruth.txt -I images.txt -o result.txt -- <tracker command>

This command will load the groundtruth and use it to initialize the tracker, then it will monitor the tracker until the end of the sequence and storing the resulting trajectory to `result.txt`. More complex behavior can be achieved by adding other command-line flags (run :code:`traxclient -h` to get their list).

OpenCV conversions
------------------

`OpenCV <http://opencv.org/>`_ is one of most frequently used C++ libraries in computer vision. This support library provides conversion functions so that protocol image and region objects can be quickly converted to corresponding OpenCV objects and vice-versa.

To compile the module you have to enable the ``BUILD_OPENCV`` flag in CMake build system (check out the :doc:`tutorial on compiling the project </tutorial_compiling>` for more details).

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
