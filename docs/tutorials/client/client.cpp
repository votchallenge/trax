/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a simple TraX client. The main function of this
 * example is to show the developers how to write new TraX clients for different
 * use cases. The examples utilizes client support library for tracker process
 * handling.
 *
 * Copyright (c) 2017, Luka Cehovin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 *
 */

// System utilities
#include <sstream>

// TraX headers
#include <trax.h>
#include <trax/client.hpp>

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

	trax::TrackerProcess tracker(tracker_command);

	for (int i = 0; i < total_frames; i++) {

		char buff[32];
		snprintf(buff, sizeof(buff), "%08d.jpg", i + 1);
		trax::ImageList images = trax::ImageList();
		trax::Image image = trax::Image::create_path(buff);
		images.set(image, TRAX_CHANNEL_COLOR);

		// Initialize every 50 frames
		if (i % 50 == 0) {
			if (!tracker.initialize(images, groundtruth[i]))
				// Something went wrong, break the loop
				break;
		} else {
			if (!tracker.frame(images))
				// Something went wrong, break the loop
				break;
		}

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

