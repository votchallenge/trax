/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This example is based on the camshiftdemo.c code from the OpenCV repository
 * It compiles with OpenCV 2.1
 * The main function of this example is to show the developers how to integrate libtrax
 * into their own programs.
 */

#ifdef _CH_
#pragma package <opencv>
#endif

#define CV_NO_BACKWARD_COMPATIBILITY

#ifndef _EiC
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>
#endif

#include "trax.h"

IplImage *image = 0, *hsv = 0, *hue = 0, *mask = 0, *backproject = 0, *histimg = 0;
CvHistogram *hist = 0;

int track_object = 0;
CvRect selection;
CvRect track_window;
CvBox2D track_box;
CvConnectedComp track_comp;
int hdims = 16;
float hranges_arr[] = {0,180};
float* hranges = hranges_arr;
int vmin = 10, vmax = 256, smin = 30;

CvScalar hsv2rgb( float hue )
{
    int rgb[3], p, sector;
    static const int sector_data[][3]=
        {{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
    hue *= 0.033333333333333333333333333333333f;
    sector = cvFloor(hue);
    p = cvRound(255*(hue - sector));
    p ^= sector & 1 ? 255 : 0;

    rgb[sector_data[sector][0]] = 255;
    rgb[sector_data[sector][1]] = 0;
    rgb[sector_data[sector][2]] = p;

    return cvScalar(rgb[2], rgb[1], rgb[0],0);
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


int main( int argc, char** argv )
{

    trax_handle* trax;
    trax_configuration config;
    config.format_region = TRAX_REGION_RECTANGLE;
    config.format_image = TRAX_IMAGE_PATH;

    // Call trax_server_setup to initialize trax protocol
    trax = trax_server_setup(config, NULL, 0);

    trax_image* img = NULL;
    trax_region* rect = NULL;

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

            // The bounding box of the object is given during initialization
            selection.x = rect->data.rectangle.x;
            selection.y = rect->data.rectangle.y;
            selection.width = rect->data.rectangle.width;
            selection.height = rect->data.rectangle.height;  

            // With every parameter master program can also give one or more key-value
            // parameters. This is useful for seting some tracking parameters externally.

            vmin = trax_properties_get_int(prop, "vmin", vmin);
            vmax = trax_properties_get_int(prop, "vmax", vmax);
            smin = trax_properties_get_int(prop, "smin", smin);

            track_object = -1;

            // properties

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
        IplImage* frame = cvLoadImage(img->data, CV_LOAD_IMAGE_COLOR);

        int i, bin_w, c;

        if( !frame )
            break;

        if( !image )
        {
            /* allocate all the buffers */
            image = cvCreateImage( cvGetSize(frame), 8, 3 );
            image->origin = frame->origin;
            hsv = cvCreateImage( cvGetSize(frame), 8, 3 );
            hue = cvCreateImage( cvGetSize(frame), 8, 1 );
            mask = cvCreateImage( cvGetSize(frame), 8, 1 );
            backproject = cvCreateImage( cvGetSize(frame), 8, 1 );
            hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
            histimg = cvCreateImage( cvSize(320,200), 8, 3 );
            cvZero( histimg );
        }

        cvCopy( frame, image, 0 );
        cvCvtColor( image, hsv, CV_BGR2HSV );

        if( track_object )
        {
            int _vmin = vmin, _vmax = vmax;

            cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),
                        cvScalar(180,256,MAX(_vmin,_vmax),0), mask );
            cvSplit( hsv, hue, 0, 0, 0 );

            if( track_object < 0 )
            {
                float max_val = 0.f;
                cvSetImageROI( hue, selection );
                cvSetImageROI( mask, selection );
                cvCalcHist( &hue, hist, 0, mask );
                cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
                cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
                cvResetImageROI( hue );
                cvResetImageROI( mask );
                track_window = selection;
                track_object = 1;

                cvZero( histimg );
                bin_w = histimg->width / hdims;
                for( i = 0; i < hdims; i++ )
                {
                    int val = cvRound( cvGetReal1D(hist->bins,i)*histimg->height/255 );
                    CvScalar color = hsv2rgb(i*180.f/hdims);
                    cvRectangle( histimg, cvPoint(i*bin_w,histimg->height),
                                 cvPoint((i+1)*bin_w,histimg->height - val),
                                 color, -1, 8, 0 );
                }
            }

            cvCalcBackProject( &hue, backproject, hist );
            cvAnd( backproject, mask, backproject, 0 );
            cvCamShift( backproject, track_window,
                        cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                        &track_comp, &track_box );
            track_window = track_comp.rect;

            if( !image->origin )
                track_box.angle = -track_box.angle;
            cvEllipseBox( image, track_box, CV_RGB(255,0,0), 3, CV_AA, 0 );
        }

        // At the end of single frame processing we send back the estimated
        // bounding box and wait for further instructions.

        CvRect result = box2rect(track_box);
        trax_region* region = trax_region_create_rectangle(result.x, result.y, result.width, result.height);

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

#ifdef _EiC
main(1,"camshiftdemo.c");
#endif
