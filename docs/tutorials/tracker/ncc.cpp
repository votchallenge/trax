/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a simple NCC tracker. The main function of this
 * example is to show the developers how to modify their trackers to integrate
 * TraX support into their trackers. The examples utilizes OpenCV library for
 * most of image processing.
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

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

/*
#include <trax.h>
#include <trax/opencv.hpp>
*/

// =========== Part 1: Tracker implementation ====================
//
// The C++ trackers are typically structured in a class to separate their private state from
// the main application. This is also recommended according to TraX integration guidelines.
class NCCTracker {
public:

	/* The initialization function receives an image and an initialization region.
	*  It initializes the internal state of the tracker.
	*/
	void init(cv::Mat img, cv::Rect rect) {
		p_window = (float) MAX(rect.width, rect.height) * 2;

		cv::Mat gray;
		cv::cvtColor(img, gray, CV_BGR2GRAY);

		int left = MAX(rect.x, 0);
		int top = MAX(rect.y, 0);

		int right = MIN(rect.x + rect.width, gray.cols - 1);
		int bottom = MIN(rect.y + rect.height, gray.rows - 1);

		cv::Rect roi(left, top, right - left, bottom - top);

		gray(roi).copyTo(p_template);

		p_position.x = (float)rect.x + (float)rect.width / 2;
		p_position.y = (float)rect.y + (float)rect.height / 2;

		p_size = cv::Size2f((float)rect.width, (float)rect.height);

		// We can keep the same debug messages in the code even after
		// integration of the TraX protocol.
		std::cout << "Tracker initialized" << std::endl;

	}

	/* The update function receives a new image in the sequence and returns the region
	*  of the object in it.
	*/
	cv::Rect update(cv::Mat img) {

		cv::Mat gray;
		cv::cvtColor(img, gray, CV_BGR2GRAY);

		float left = MAX(round(p_position.x - (float)p_window / 2), 0);
		float top = MAX(round(p_position.y - (float)p_window / 2), 0);

		float right = MIN(round(p_position.x + (float)p_window / 2), gray.cols - 1);
		float bottom = MIN(round(p_position.y + (float)p_window / 2), gray.rows - 1);

		cv::Rect roi((int) left, (int) top, (int) (right - left), (int) (bottom - top));

		if (roi.width < p_template.cols || roi.height < p_template.rows) {
			cv::Rect result;

			result.x = (int) (p_position.x - p_size.width / 2);
			result.y = (int)(p_position.y - p_size.height / 2);
			result.width = (int)(p_size.width);
			result.height = (int)(p_size.height);
			return result;

		}

		cv::Mat matches;
		cv::Mat cut = gray(roi);

		cv::matchTemplate(cut, p_template, matches, CV_TM_CCOEFF_NORMED);

		cv::Point matchLoc;
		cv::minMaxLoc(matches, NULL, NULL, NULL, &matchLoc, cv::Mat());

		cv::Rect result;

		p_position.x = left + matchLoc.x + (float)p_size.width / 2;
		p_position.y = top + matchLoc.y + (float)p_size.height / 2;

		result.x = (int)(left + matchLoc.x);
		result.y = (int)(top + matchLoc.y);
		result.width = p_size.width;
		result.height = p_size.height;

		// We can keep the same debug messages in the code even after
		// integration of the TraX protocol.
		std::cout << "Tracker updated" << std::endl;

		return result;
	}

private:

	cv::Point2f p_position;
	cv::Size p_size;
	float p_window;
	cv::Mat p_template;

};

// =========== Part 2: Utility functions ====================

// Utility function that reads the initialization region from a file and
// returns OpenCV rectangle
cv::Rect readrectangle(const std::string file) {

	std::ifstream ifs (file.c_str(), std::ifstream::in);
	if (!ifs.is_open()) {
		std::cerr << "Initial region file not available. Stopping." << std::endl;
		exit(-1);
	}

	cv::Rect rect;
	std::string token;

    std::getline(ifs, token, ',');
	rect.x = atoi(token.c_str());
    std::getline(ifs, token, ',');
	rect.y = atoi(token.c_str());
    std::getline(ifs, token, ',');
	rect.width = atoi(token.c_str());
    std::getline(ifs, token, ',');
	rect.height = atoi(token.c_str());

	return rect;
}

// Utility function that reads the sequence of image file paths from a file
// and returns them in form of a string vector.
std::vector<std::string> readsequence(const std::string file) {

	std::ifstream ifs (file.c_str(), std::ifstream::in);
	if (!ifs.is_open()) {
		std::cerr << "Image sequence file not available. Stopping." << std::endl;
		exit(-1);
	}

	std::vector<std::string> sequence;
	std::string path;

	while (true) {
		getline(ifs, path, '\n');
		if (path.empty())
			break;
		sequence.push_back(path);
	}

	return sequence;
}

// Utility function that writes the resulting trajectory to a file.
void writerectangles(const std::string file, const std::vector<cv::Rect>& rectangles) {

	std::ofstream ofs (file.c_str(), std::ofstream::out);
	if (!ofs.is_open()) {
		std::cerr << "Cannot write to file. Stopping." << std::endl;
		exit(-1);
	}

	for (size_t i = 0; i < rectangles.size(); i++) {
		ofs << rectangles[i].x << "," << rectangles[i].y << "," <<
		    rectangles[i].width << "," << rectangles[i].height << std::endl;
	}

}

// =========== Part 3: Tracking loop ====================
//
// The main function of the program that also contains the tracking loop. The loop should
// be well separated from the tracker's implementation.
int main( int argc, char** argv) {

	// Reading
	std::vector<std::string> images = readsequence("images.txt");
	cv::Rect initialization = readrectangle("region.txt");
	std::vector<cv::Rect> output;

	NCCTracker tracker;

	// Initializing the tracker with first image
	cv::Mat image = cv::imread(images[0]);
	tracker.init(image, initialization);
	// Adding first output to maintain equal length of outputs and
	// input images.
	output.push_back(initialization);

	// The tracking loop, iterating through the rest of the sequence
	for (size_t i = 1; i < images.size(); i++) {
		cv::Mat image = cv::imread(images[i]);
		cv::Rect rect = tracker.update(image);
		output.push_back(rect);

	}

	// Writing the tracking result and exiting
	writerectangles("output.txt", output);

}

/*
int main( int argc, char** argv) {

	NCCTracker tracker;

	trax::Server handle(trax::Configuration(TRAX_IMAGE_PATH | TRAX_IMAGE_MEMORY |
	                                        TRAX_IMAGE_BUFFER, TRAX_REGION_RECTANGLE), trax_no_log);

	while (true) {

		trax::Image image;
		trax::Region region;
		trax::Properties properties;

		int tr = handle.wait(image, region, properties);
		if (tr == TRAX_INITIALIZE) {

			tracker.init(trax::image_to_mat(image), trax::region_to_rect(region));

			handle.reply(region, trax::Properties());

		} else if (tr == TRAX_FRAME) {

			cv::Rect result = tracker.update(image_to_mat(image));
			handle.reply(trax::rect_to_region(result), trax::Properties());

		}
		else break;
	}

}
*/

