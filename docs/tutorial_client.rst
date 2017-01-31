Creating a TraX client
======================

According to TraX specification a client is an application that provides tracking task to the tracker, i.e. the server. The client typically has to start the server process, initialize the tracker, provide image sequence data and monitor the tracker's progress. Examples of fully functional clients in the TraX project are the CLI utility `traxclient` which can be used to perform simple experiments on a given sequence and `traxplayer` which can be used to interactively initialize the tracker and run it on the data from a video or even camera.

In this tutorial we will create a client that runs a tracker for 50 frames (the sequence is loaded from a given directory) and automatically reinitializes it after that. We will utilize the TraX client support library that takes care of common and platform specific tasks like managing the tracker process. The complete source code of the example is available in `docs/tutorials/client` directory of the repository.

To include the client support we first have to include its header file (besides all other header files):

.. code-block:: c++

	// System utilities
	#include <sstream>

	// TraX headers
	#include <trax.h>
	#include <trax/client.hpp>

We will begin by parsing the input arguments, provided by the user. For simplicity sake we will assume that the input sequence is defined by the `groundtruth.txt` file which defines the annotated groundtruth state and that the image files are also available in the same folder and follow a eight digit zero-padded number, starting with 1: `00000001.jpg, 00000002.jpg, ...`. The only remaining input is the tracker command which we have to parse from the input arguments and encode it as a string:

.. code-block:: c++

	int main( int argc, char** argv) {

		// Quit if tracker command not given
		if (argc < 2)
			return -1;

		std::vector<trax::Region> groundtruth;
		trax::load_trajectory("groundtruth.txt", groundtruth);

		int total_frames = groundtruth.size();

		std::stringstream buffer;
		for (int i = 1; i < argc; i++)
			buffer << " \"" << std::string(argv[i]) << "\" ";
		std::string tracker_command = buffer.str();

To obtain tracker handle we create a `TrackerProcess` object. This object internally starts a new process using the given command parameter. The construction of the object will faill if the tracker will not send an introduction TraX message which will also provide client with the information about tracker's capabilites (what kind of image formats does it support). We can also check if the process is still alive by calling `ready` method.

.. code-block:: c++

		trax::TrackerProcess tracker(tracker_command);

For now we will leave these advanced options and look at the client tracking loop. The client is aware of the length of the sequence so the loop iterates for a known number of steps. At each step we generate a path image descriptor to the new image. The client has complete control over what is going on which we can use to re-initialize the tracker every 50 frames.

.. code-block:: c++

		for (int i = 0; i < total_frames; i++) {

			char buff[32];
			snprintf(buff, sizeof(buff), "%08d.jpg", i + 1);
			trax::Image image = trax::Image::create_path(string(buff));

			// Initialize every 50 frames
			if (i % 50 == 0) {
				if (!tracker.initialize(image, groundtruth[i]))
					// Something went wrong, break the loop
					break;
			} else {
				if (!tracker.frame(image))
					// Something went wrong, break the loop
					break;
			}


After issuing the command we wait for the reply. Unless the tracker terminates execution, a valid reply will contain the status of the object as well as optional properties. All of this information can be simply stored or used to adjust tracking session.

.. code-block:: c++

			trax::Region status; // Will receive object state predicted by tracker
			trax::Properties properties; // Will receive any optional data

			// Wait for tracker response to the request
			bool success = tracker.wait(status, properties);

			if (success) {
				// The tracker returns a valid status.
				// Process the result ...
			} else {
				if (tracker.ready()) {
					// The tracker has requested termination.
					break;
				} else {
					// In case of an error ...
				}
			}

		}


	}

And all we have to do now is to compile the code. We will use CMake to find the TraX library and configure the build. Notice that we have to tell CMake to include ´core´ and ´client´ TraX components::

	ADD_EXECUTABLE(sample_client client.cpp)
	FIND_PACKAGE(trax REQUIRED COMPONENTS core client)
	TARGET_LINK_LIBRARIES(sample_client ${TRAX_LIBRARIES})
	INCLUDE_DIRECTORIES(AFTER ${TRAX_INCLUDE_DIRS})
	LINK_DIRECTORIES(AFTER ${TRAX_LIBRARY_DIRS})

