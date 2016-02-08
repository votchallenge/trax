/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This example is based on the camshiftdemo code from the OpenCV repository
 * The main function of this example is to show the developers how to integrate trax
 * support into their own programs.
 */

//#include <opencv2/tracking.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>

#include <iostream>

#include "trax.h"

using namespace std;
using namespace cv;

Rect trackWindow;
int hsize = 16;
float hranges[] = {0,180};
const float* phranges = hranges;

Mat read_image(trax_image* input) {

    switch (trax_image_get_type(input)) {
    case TRAX_IMAGE_PATH:
        return imread(string(trax_image_get_path(input)), CV_LOAD_IMAGE_COLOR);
    case TRAX_IMAGE_MEMORY: {
        int width, height, format;
        trax_image_get_memory_header(input, &width, &height, &format);
        char* data = trax_image_get_memory_row(input, 0);

        Mat tmp(height, width, format == TRAX_IMAGE_MEMORY_RGB ? CV_8UC3 : 
            (format == TRAX_IMAGE_MEMORY_GRAY8 ? CV_8UC1 : CV_16U), data);

        Mat result;

        cv::cvtColor(tmp, result, CV_BGR2RGB);
        
        return result;
    }
    case TRAX_IMAGE_BUFFER: {
        int length, format;
        const char* buffer = trax_image_get_buffer(input, &length, &format);

        // This is problematic but we will just read from the buffer anyway ...
        const Mat mem(1, length, CV_8UC1, const_cast<char *>(buffer)); 

        return imdecode(mem, CV_LOAD_IMAGE_COLOR);

    }
    }
    return cv::Mat();
}

CvRect box2rect(CvBox2D box) {

    CvPoint pt0, pt1, pt2, pt3;
    CvRect result;

    double _angle = box.angle*CV_PI/180.0; 
    float a = (float)cos(_angle)*0.5f; 
    float b = (float)sin(_angle)*0.5f; 
     
    pt0.x = box.center.x - a*box.size.height - b*box.size.width; 
    pt0.y = box.center.y + b*box.size.height - a*box.size.width; 
    pt1.x = box.center.x + a*box.size.height - b*box.size.width; 
    pt1.y = box.center.y - b*box.size.height - a*box.size.width; 
    pt2.x = 2 * box.center.x - pt0.x; 
    pt2.y = 2 * box.center.y - pt0.y; 
    pt3.x = 2 * box.center.x - pt1.x; 
    pt3.y = 2 * box.center.y - pt1.y; 

    result.x = cvFloor(MIN(MIN(MIN(pt0.x, pt1.x), pt2.x), pt3.x)); 
    result.y = cvFloor(MIN(MIN(MIN(pt0.y, pt1.y), pt2.y), pt3.y));

    result.width = cvCeil(MAX(MAX(MAX(pt0.x, pt1.x), pt2.x), pt3.x)) - result.x - 1; 
    result.height = cvCeil(MAX(MAX(MAX(pt0.y, pt1.y), pt2.y), pt3.y)) - result.y - 1; 

    return result; 
}

int track_object = 0;
Point origin;
Rect selection;
int vmin = 10, vmax = 256, smin = 30;

int main( int argc, char** argv )
{

    trax_handle* trax;
    trax_configuration config;
    config.format_region = TRAX_REGION_RECTANGLE;
    config.format_image = TRAX_IMAGE_MEMORY;

    // Call trax_server_setup to initialize trax protocol
    trax = trax_server_setup(config, NULL);

    trax_image* img = NULL;
    trax_region* rect = NULL;

    Mat frame, hsv, hue, mask, hist, backproj;

    for(;;)
    {
        trax_properties* prop = trax_properties_create();

        // The main idea of Trax interface is to leave the control to the master program
        // and just follow the instructions that the tracker gets. 
        // The main function for this is trax_wait that actually listens for commands.

        int tr = trax_server_wait(trax, &img, &rect, prop);

        // There are two important commands. The first one is TRAX_INITIALIZE that tells the
        // tracker how to initialize.
        if (tr == TRAX_INITIALIZE) {

            float x, y, width, height;

            trax_region_get_rectangle(rect, &(x), &(y),
                &(width), &(height));

            // The bounding box of the object is given during initialization
            selection.x = x;
            selection.y = y;
            selection.width = width;
            selection.height = height;  

            // With every parameter master program can also give one or more key-value
            // parameters. This is useful for seting some tracking parameters externally.

            vmin = trax_properties_get_int(prop, "vmin", vmin);
            vmax = trax_properties_get_int(prop, "vmax", vmax);
            smin = trax_properties_get_int(prop, "smin", smin);

            track_object = -1;

            trax_server_reply(trax, rect, NULL);
        } else
        // The second one is TRAX_FRAME that tells the tracker what to process next.
        if (tr == TRAX_FRAME) {

            if (track_object < 1) {
                // Tracker was not initialized successfully
                trax_properties_release(&prop);
                break;
            }
        } 
        // Any other command is either TRAX_QUIT or illegal, so we exit.
        else {
            trax_properties_release(&prop);
            break;
        }

        trax_properties_release(&prop);

        // In trax mode images are read from the disk. The master program tells the
        // tracker where to get them.
        Mat frame = read_image(img);

        imshow("Text", frame);
        waitKey(1);

        int i, bin_w, c;

        if( frame.empty() )
            break;

        cvtColor(frame, hsv, COLOR_BGR2HSV);

        if( track_object )
        {

            int _vmin = vmin, _vmax = vmax;

            inRange(hsv, Scalar(0, smin, MIN(_vmin,_vmax)),
                    Scalar(180, 256, MAX(_vmin, _vmax)), mask);
            int ch[] = {0, 0};
            hue.create(hsv.size(), hsv.depth());
            mixChannels(&hsv, 1, &hue, 1, ch, 1);

            if( track_object < 0 )
            {
                Mat roi(hue, selection), maskroi(mask, selection);
                calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
                normalize(hist, hist, 0, 255, NORM_MINMAX);

                trackWindow = selection;
                track_object = 1;

                Mat buf(1, hsize, CV_8UC3);
                for( int i = 0; i < hsize; i++ )
                    buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./hsize), 255, 255);
                cvtColor(buf, buf, COLOR_HSV2BGR);

            }

            calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
            backproj &= mask;
            RotatedRect trackBox = CamShift(backproj, trackWindow,
                                TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));

            if( trackWindow.area() <= 1 )
            {
                int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                                   trackWindow.x + r, trackWindow.y + r) &
                              Rect(0, 0, cols, rows);
            }

        }

        // At the end of single frame processing we send back the estimated
        // bounding box and wait for further instructions.
        trax_region* region = trax_region_create_rectangle(trackWindow.x, trackWindow.y, trackWindow.width, trackWindow.height);

        // Note that the tracker also has an option of sending additional data
        // back to the main program in a form of key-value pairs. We do not use
        // this option here, so this part is empty.
        trax_server_reply(trax, region, NULL);

        trax_region_release(&region);

        if (img) trax_image_release(&img);
        if (rect) trax_region_release(&rect);

    }

    if (img) trax_image_release(&img);
    if (rect) trax_region_release(&rect);

    // Call trax_cleanup to release potentially allocated resources 
    trax_cleanup(&trax);

    return 0;
}

