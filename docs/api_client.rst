Client library
==============

The client support library provides a C++ client class that uses C++ protocol API to communicate with the tracker process, besides communication the class also takes care of launching tracker process, handling timeouts, logging, and other things. The class also supports setting up communication over streams as well as over TCP sockets. A detailed tutorial on how to use the client library is available :doc:`here </tutorial_client>`. To compile the module you have to enable ``BUILD_CLIENT`` flag in CMake build system (check out the :doc:`tutorial on compiling the project </tutorial_compiling>` for more details).

.. cpp:namespace:: trax

.. c:macro:: CONNECTION_DEFAULT

    Default connection mode, standard input and output streams.

.. c:macro:: CONNECTION_EXPLICIT

    Non-standard input and output streams are created, passed to tracker via environment variables :c:macro:`TRAX_IN` and :c:macro:`TRAX_OUT`.

.. c:macro:: CONNECTION_SOCKETS

    Using TCP socket for communication, connection parameters passed to tracker via environment variable :c:macro:`TRAX_SOCKET`.

.. cpp:class:: TrackerProcess

	.. cpp:function:: TrackerProcess(const std::string& command, const std::map<string, string> environment, int timeout = 10, trax::ConnectionMode connection = trax::CONNECTION_DEFAULT, VerbosityMode verbosity = trax::VERBOSITY_DEFAULT)

		:param command: The name of tracker program followed by its input arguments
		:param environment: A map of environmental variables that have to be set for the tracker process
		:param timeout: Number of seconds to wait for tracker's response before terminating the session
		:param connection: Type of connection, supported are either :c:macro:`CONNECTION_DEFAULT`, :c:macro:`CONNECTION_EXPLICIT`, or :c:macro:`CONNECTION_SOCKETS`.
		:param verbosity: How verbose should the output be

	.. cpp:function:: ~TrackerProcess()

		Destroys the process and cleans up data.

	.. cpp:function:: int image_formats()

		:returns: A bitset of image formats supported by the tracker.

	.. cpp:function:: int region_formats()

		:returns: A bitset of region formats supported by the tracker.

	.. cpp:function:: bool ready()

		:returns: True if the tracker process is alive.

	.. cpp:function:: bool tracking()

		:returns: True if the tracker process is tracking an object (was initalized).

	.. cpp:function:: bool initialize(const trax::Image& image, const trax::Region& region, const trax::Properties& properties = trax::Properties())

		Request tracker initialization with a given image and region.

		:param image: Input image object
		:param region: Input region object
		:param properties: Optional properties passed to the tracker
		:returns: True on success, if the function returns false check :cpp:func:`trax::TrackerProcess::ready` if the process has crashed or simply requested termination

	.. cpp:function:: bool frame(const trax::Image& image, const trax::Properties& properties = trax::Properties())

		Request tracker update with a given image.

		:param image: Input image object
		:param properties: Optional properties passed to the tracker
		:returns: True on success, if the function returns false check :cpp:func:`trax::TrackerProcess::ready` if the process has crashed or simply requested termination

	.. cpp:function:: bool wait(trax::Region& region, trax::Properties& properties)

		Waits for tracker's response to the previous request (either initialization or update).

		:param region: Output region object, on success populated with output region
		:param properties: Optional output properties object, populated by returned optional values, if any
		:returns: True on success, if the function returns false check :cpp:func:`trax::TrackerProcess::ready` if the process has crashed or simply requested termination

	.. cpp:function:: bool reset()

		Restarts the tracker process. The function terminates the tracker process and starts a new one.

.. cpp:function:: int load_trajectory(const std::string& file, std::vector<Region>& trajectory)

   Utility function to load a trajectory (a sequence of object states) form a text file.

   :param file: Filename string
   :param trajectory: Empty vector that will be populated with region states
   :return: Number of read states

.. cpp:function:: void save_trajectory(const std::string& file, std::vector<Region>& trajectory)

   Utility function to save a trajectory (a sequence of object states) to a text file.

   :param file: Filename string
   :param trajectory: Vector that contains the trajectory
