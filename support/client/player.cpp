
#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdarg.h>
#include <trax/client.hpp>
#include <trax/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <stdexcept>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#define NOMINMAX
#include <ctype.h>
#include <windows.h>
__inline void sleep(long time) {
    Sleep(time * 1000);
}
#else
#ifdef _MAC_
#else
#include <unistd.h>
#endif
#include <signal.h>
#endif

#include <time.h>

#include "getopt.h"

using namespace cv;
using namespace std;
using namespace trax;

#define WINDOW_NAME "TraX Player"

ConnectionMode connection = CONNECTION_DEFAULT;
VerbosityMode verbosity = VERBOSITY_DEFAULT;

void print_debug(const char *format, ...) {
    if (verbosity != VERBOSITY_DEBUG)
        return;

    va_list args;
    va_start(args, format);

    fprintf(stdout, "CLIENT: ");
    vfprintf(stdout, format, args);

    va_end(args);
}

Image convert_image(Mat& sensor, int formats) {

    Image image;

    if TRAX_SUPPORTS(formats, TRAX_IMAGE_BUFFER) {
        vector<uchar> buffer;
        imencode(".jpg", sensor, buffer);
        image = Image::create_buffer(buffer.size(), (char*) &buffer[0]);
    } else if TRAX_SUPPORTS(formats, TRAX_IMAGE_MEMORY) {
        image = trax::mat_to_image(sensor);
    } else
        throw std::runtime_error("No supported image format allowed");

    return image;

}

typedef struct RegionCapture {
    bool dragging;
    Point start;
    Point current;
    Region region;
} RegionCapture;

void on_mouse(int event, int x, int y, int flags, void* data) {

    Point point(x, y);

    RegionCapture* rc = (RegionCapture *) data;

    if ( event == EVENT_LBUTTONDOWN ) {
        rc->dragging = true;
        rc->start = point;
    } else if ( event == EVENT_LBUTTONUP ) {
        rc->dragging = false;
        rc->region = Region::create_rectangle(rc->start.x, rc->start.y, point.x - rc->start.x, point.y - rc->start.y);
    }

    if (rc->dragging) {
        rc->current = point;
    }

}

Region get_region_interactive(const string& window, Mat& image) {

    RegionCapture rc;
    rc.dragging = false;
    bool run = true;

    string message("Drag mouse to enter region and SPACE to confirm, press Q to cancel input.");

    Mat buffer;

    setMouseCallback(window, on_mouse, &rc);

    while (run) {

        image.copyTo(buffer);

        if (rc.dragging) {
            rectangle(buffer, rc.start, rc.current, Scalar(0, 255, 0), 3);
        }

        if (!rc.region.empty()) {
            draw_region(buffer, rc.region, Scalar(0, 255, 0), 1);
        }

        putText(buffer, message, Point(10, 10), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 255, 0));
        imshow(window, buffer);

        int i = waitKey(25);

        switch ((char)i) {
        case 'q': {
            rc.region = Region();
            break;
        }
        case ' ': {
            run = false;
            break;
        }
        }

    }

    setMouseCallback(window, NULL, NULL);

    return rc.region;
}


bool read_frame(VideoCapture& reader, int frame, Mat& image) {

    int current = reader.get(CV_CAP_PROP_POS_FRAMES);

    if (frame < current) return false;

    while (current != frame + 1) {
        reader.read(image);
        current++;
    }

    return !image.empty();

}

void configure_signals() {

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

    // TODO: anything to do here?

#else

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags =  SA_RESTART | SA_NOCLDSTOP; //SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror(0);
        exit(1);
    }

#endif

}

#define GROUNDTRUTH_COLOR Scalar(100, 255, 100)
#define TRACKER_COLOR Scalar(100, 100, 255)

#define CMD_OPTIONS "hdV:G:t:p:e:xXg"

#ifndef TRAX_BUILD_DATE
#define TRAX_BUILD_DATE __DATE__
#endif

void print_help() {

    cout << "OpenCV Video capture player client" << "\n\n";
    cout << "Built on " << TRAX_BUILD_DATE << "\n";
    cout << "TraX library version: " << trax_version() << "\n";
    cout << "Protocol version: " << TRAX_VERSION << "\n\n";

    cout << "Usage: traxplayer [-h] [-d]  \n";
    cout << "\t [-e name=value] [-V video_file]\n";
    cout << "\t [-p name=value] [-t timeout] [-x] [-X]\n";
    cout << "\t [-g] -- <command_part1> <command_part2> ...";

    cout << "\n\nProgram arguments: \n";
    cout << "\t-d\tEnable debug\n";
    cout << "\t-e\tEnvironmental variable (multiple occurences allowed)\n";
    cout << "\t-g\tVisualize result in a window\n";
    cout << "\t-h\tPrint this help and exit\n";
    cout << "\t-V\tVideo file for the input cubemap sequence.\n";
    cout << "\t-p\tTracker parameter (multiple occurences allowed)\n";
    cout << "\t-t\tSet timeout period\n";
    cout << "\t-x\tUse explicit streams, not standard ones.\n";
    cout << "\t-X\tUse TCP/IP sockets instead of file streams.\n";
    cout << "\n";

    cout << "\n";
}

int main(int argc, char** argv) {

    int c;
    opterr = 0;
    int result = 0;
    int timeout = 30;

    string timing_file;
    string tracker_command;
    string video_file("input.avi");

    Properties properties;
    map<string, string> environment;

    configure_signals();

    try {

        while ((c = getopt(argc, argv, CMD_OPTIONS)) != -1)
            switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'd':
                verbosity = VERBOSITY_DEBUG;
                break;
            case 'x':
                connection = CONNECTION_EXPLICIT;
                break;
            case 'X':
                connection = CONNECTION_SOCKETS;
                break;
            case 'V':
                video_file = string(optarg);
                break;
            case 't':
                timeout = MAX(0, atoi(optarg));
                break;
            case 'e': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                environment[string(var, ptr - var)] = string(ptr + 1);
                break;
            }
            case 'p': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                string key(var, ptr - var);
                string value(ptr + 1);
                properties.set(key, value);
                break;
            }
            default:
                print_help();
                throw std::runtime_error(string("Unknown switch -") + string(1, (char) optopt));
            }

        if (optind < argc) {

            stringstream buffer;

            for (int i = optind; i < argc; i++) {
                buffer << " \"" << string(argv[i]) << "\" ";
            }

            tracker_command = buffer.str();

        } else {
            print_help();
            exit(-1);
        }

        Mat cvimage;
        VideoCapture reader(video_file);
        int video_length = reader.get(CV_CAP_PROP_FRAME_COUNT);

        print_debug("Video will be loaded from file %s.\n", video_file.c_str());

        namedWindow(WINDOW_NAME);

        print_debug("Sequence length: %d frames.\n", video_length);

        TrackerProcess tracker(tracker_command, environment, timeout, connection, verbosity);

        int frame = 0;
        while (true) {

            Region initialization_region;

            if (!read_frame(reader, frame, cvimage)) {
                print_debug("No data available. Stopping. \n");
                break;
            }
            initialization_region = get_region_interactive(WINDOW_NAME, cvimage);
            if (!initialization_region.empty()) {
                print_debug("Using interactive region: %s\n", ((string) initialization_region).c_str());
            } else break;

            if (!tracker.ready()) {
                throw std::runtime_error("Tracker process not alive anymore.");
            }

            Metadata metadata = tracker.metadata();

            ImageList image;
            image.set(convert_image(cvimage, metadata.image_formats()), TRAX_CHANNEL_COLOR);

            if (!tracker.initialize(image, initialization_region, properties)) {
                throw std::runtime_error("Unable to initialize tracker.");
            }

            draw_region(cvimage, initialization_region, GROUNDTRUTH_COLOR, 3);
            imshow(WINDOW_NAME, cvimage);
            waitKey(25);

            while (frame < video_length) {
                // Repeat while tracking the target.
                Region status;
                Properties additional;

                bool result = tracker.wait(status, additional);

                if (result) {
                    // Default option, the tracker returns a valid status.

                    Region storage;

                } else {
                    if (tracker.ready()) {
                        // The tracker has requested termination of connection.
                        print_debug("Termination requested by tracker.\n");
                        break;
                    } else {
                        // In case of an error ...
                        throw std::runtime_error("Unable to contact tracker.");
                    }
                }

                draw_region(cvimage, status, TRACKER_COLOR, 1);
                imshow(WINDOW_NAME, cvimage);
                waitKey(25);

                frame++;

                if (!read_frame(reader, frame, cvimage)) {
                    print_debug("No more data available. Stopping. \n");
                    break;
                }

                image.set(convert_image(cvimage, metadata.image_formats()), TRAX_CHANNEL_COLOR);

                Properties no_properties;
                if (!tracker.frame(image, no_properties))
                    throw std::runtime_error("Unable to send new frame.");

            }

        }

        reader.release();

    } catch (const std::runtime_error &e) {

        fprintf(stderr, "Error: %s\n", e.what());
        result = -1;

    }

    exit(result);

}
