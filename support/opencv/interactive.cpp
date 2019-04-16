/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a stationary tracker. It only reports the initial 
 * position for all frames and is used for testing purposes.
 * The main function of this example is to show the developers how to modify
 * their trackers to work with the evaluation environment.
 *
 * Copyright (c) 2016, Luka Cehovin
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

// Interactive tracker is a simple GUI application for testing where a human can click
// an object in every frame and the information is transmitted over the TraX 
// protocol to client. The purpose of the application mainly testing and demonstrative.

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <trax/opencv.hpp>
#include <trax.h>

#define WINDOW_NAME "Interactive tracker"

bool run = true;
bool wait = false;

void on_mouse(int event, int x, int y, int, void* data) {

  if( event == CV_EVENT_LBUTTONDOWN && wait ) {

    cv::Rect *rect = (cv::Rect *) data;

    rect->x = x - rect->width / 2;
    rect->y = y - rect->height / 2;

    wait = false;

  }

}

int main( int argc, char** argv)
{ 
    trax::ImageList img;
    trax::Region reg;

    trax::Configuration config(TRAX_IMAGE_PATH | TRAX_IMAGE_MEMORY | TRAX_IMAGE_BUFFER, TRAX_REGION_RECTANGLE);
    trax::Server handle(config, trax_no_log);

    cv::namedWindow(WINDOW_NAME);

    cv::Mat frame;
    cv::Rect rectangle;

    cv::setMouseCallback(WINDOW_NAME, on_mouse, &rectangle);

    while(run)
    {

        trax::Properties prop;

        int tr = handle.wait(img, reg, prop);

         if (tr == TRAX_INITIALIZE) {

            rectangle = trax::region_to_rect(reg);
            frame = trax::image_to_mat(img.get(TRAX_CHANNEL_COLOR));

        } else if (tr == TRAX_FRAME) {

            frame = trax::image_to_mat(img.get(TRAX_CHANNEL_COLOR));

        } else {
            break;
        }

        cv::rectangle(frame, rectangle.tl(), rectangle.br(), cv::Scalar(0, 0, 255));
        cv::circle(frame, cv::Point(rectangle.x + rectangle.width / 2,
            rectangle.y + rectangle.height / 2), 2, cv::Scalar(0, 0, 255));

        cv::imshow(WINDOW_NAME, frame);

        wait = true;

        while (wait) {

            if (cv::waitKey(30) > -1) {
                run = false;
                break;
            }

        }

        trax::Region status = trax::rect_to_region(rectangle);
        handle.reply(status, trax::Properties());

    }

    return 0;
}

