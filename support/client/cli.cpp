/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a stationary tracker. It only reports the initial
 * position for all frames and is used for testing purposes.
 * The main function of this example is to show the developers how to modify
 * their trackers to work with the evaluation environment.
 *
 * Copyright (c) 2015, Luka Cehovin
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

//#define _BSD_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <stdexcept>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <stdarg.h>

#include <trax/client.hpp>

#ifdef TRAX_BUILD_OPENCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <trax/opencv.hpp>
#endif

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <ctype.h>
#include <windows.h>

#define strcmpi _strcmpi

__inline void sleep(long time) {
    Sleep(time * 1000);
}

#else

#ifdef _MAC_

#else
    #include <unistd.h>
#endif

#include <signal.h>
#define strcmpi strcasecmp

#endif

#include "getopt.h"
#include "region.h"

using namespace std;
using namespace trax;

#define CMD_OPTIONS "hsdI:G:f:O:S:r:t:T:p:e:xXQ"

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

#ifndef TRAX_BUILD_DATE
#define TRAX_BUILD_DATE __DATE__
#endif

void print_help() {

    cout << "TraX Client" << "\n\n";
    cout << "Built on " << TRAX_BUILD_DATE << "\n";
    cout << "Library version: " << trax_version() << "\n";
    cout << "Protocol version: " << TRAX_VERSION << "\n\n";

    cout << "Usage: traxclient [-h] [-d] [-I image_list] [-O output_file] \n";
    cout << "\t [-f threshold] [-r frames] [-G groundtruth_file] [-e name=value] \n";
    cout << "\t [-p name=value] [-t timeout] [-s] [-T timing_file] [-x] [-X]\n";
    cout << "\t -- <command_part1> <command_part2> ...";

    cout << "\n\nProgram arguments: \n";
    cout << "\t-h\tPrint this help and exit\n";
    cout << "\t-d\tEnable debug\n";
    cout << "\t-t\tSet timeout period\n";
    cout << "\t-G\tGroundtruth annotations file\n";
    cout << "\t-I\tFile that lists the image sequence files\n";
    cout << "\t-O\tOutput region file\n";
    cout << "\t-S\tInitialization region file (if different than groundtruth)\n";
    cout << "\t-T\tOutput timings file\n";
    cout << "\t-f\tFailure threshold\n";
    cout << "\t-r\tReinitialization offset\n";
    cout << "\t-e\tEnvironmental variable (multiple occurences allowed)\n";
    cout << "\t-p\tTracker parameter (multiple occurences allowed)\n";
    cout << "\t-Q\tWait for tracker to respond, then output its information and quit.\n";
    cout << "\t-x\tUse explicit streams, not standard ones.\n";
    cout << "\t-X\tUse TCP/IP sockets instead of file streams.\n";
    cout << "\n";

    cout << "\n";
}

static bool exit_cache = false;

void handle_signal(int s) {

    exit_cache = true;

}


void configure_signals() {

    #if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

    // TODO: anything to do here?

    #else

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; //SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
      perror(0);
      exit(1);
    }

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, 0) == -1) {
      perror(0);
      exit(1);
    }

    #endif

}

bool must_exit() {

    return exit_cache;

}

#include <string.h>

typedef struct image_size {
    int width;
    int height;
} image_size;

// Based on the example from: http://www.wischik.com/lu/programmer/get-image-size.html
// Retrieve image size from image header
image_size image_get_size(Image image) {

    int len;
    image_size result;
    unsigned char buf[24];

    if (image.type() == TRAX_IMAGE_MEMORY) {

        image.get_memory_header(&result.width, &result.height, NULL);
        return result;

    }

    if (image.type() == TRAX_IMAGE_PATH) {
        const string filename = image.get_path();
        int r;

        result.width = -1;
        result.height = -1;

        FILE *f=fopen(filename.c_str(),"rb");
        if ( f==0 ) return result;

        fseek(f,0,SEEK_END);
        int len=ftell(f);
        fseek(f,0,SEEK_SET);

        if (len<24) {fclose(f); return result;}

        // Strategy:
        // reading GIF dimensions requires the first 10 bytes of the file
        // reading PNG dimensions requires the first 24 bytes of the file
        // reading JPEG dimensions requires scanning through jpeg chunks
        // In all formats, the file is at least 24 bytes big, so we'll read that always    
        r = fread(buf,1,24,f);
        if (r == -1) {
            return result;
        }

        // For JPEGs, we need to read the first 12 bytes of each chunk.
        // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
        if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF && buf[3]==0xE0 && buf[6]=='J' && buf[7]=='F' && buf[8]=='I' && buf[9]=='F') {
            long pos=2;
            while (buf[2]==0xFF) {
                if (buf[3]==0xC0 || buf[3]==0xC1 || buf[3]==0xC2 || buf[3]==0xC3 || buf[3]==0xC9 || buf[3]==0xCA || buf[3]==0xCB) break;
                pos += 2+(buf[4]<<8)+buf[5];
                if (pos+12>len) break;
                fseek(f,pos,SEEK_SET);
                r = fread(buf+2,1,12,f);
                if (r == -1) { return result; }
            }
        }

        fclose(f);
        
    } else if (image.type() == TRAX_IMAGE_BUFFER) {

        const char* src = image.get_buffer(&len, NULL);

        if (len<24) { return result; }

        memcpy(buf, src, 24);

    }

    // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
    if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF) {
        result.height = (buf[7]<<8) + buf[8];
        result.width = (buf[9]<<8) + buf[10];
    }

    // GIF: first three bytes say "GIF", next three give version number. Then dimensions
    if (buf[0]=='G' && buf[1]=='I' && buf[2]=='F') {
        result.width = buf[6] + (buf[7]<<8);
        result.height = buf[8] + (buf[9]<<8);
    }

    // PNG: the first frame is by definition an IHDR frame, which gives dimensions
    if ( buf[0]==0x89 && buf[1]=='P' && buf[2]=='N' && buf[3]=='G' && buf[4]==0x0D && buf[5]==0x0A && buf[6]==0x1A && buf[7]==0x0A
        && buf[12]=='I' && buf[13]=='H' && buf[14]=='D' && buf[15]=='R') {
        result.width = (buf[16]<<24) + (buf[17]<<16) + (buf[18]<<8) + (buf[19]<<0);
        result.height = (buf[20]<<24) + (buf[21]<<16) + (buf[22]<<8) + (buf[23]<<0);
    }

  return result;
}

image_size image_get_size(ImageList image) {

    if (image.has(TRAX_CHANNEL_COLOR)) {
        return image_get_size(image.get(TRAX_CHANNEL_COLOR));
    }

    if (image.has(TRAX_CHANNEL_DEPTH)) {
        return image_get_size(image.get(TRAX_CHANNEL_DEPTH));
    }

    if (image.has(TRAX_CHANNEL_IR)) {
        return image_get_size(image.get(TRAX_CHANNEL_IR));
    }

    image_size size;
    return size;
}


#define DELIMITER ";"

void read_images(const string& file, vector<vector<string> >& list) {

    std::ifstream input;

    input.open(file.c_str(), std::ifstream::in);

    if (!input.is_open())
        throw std::runtime_error("Image list file not available.");

    string delimiter(DELIMITER);

    while (true) {
        string line;
        vector<string> images;
        getline(input, line);
        if (!input.good()) break;

        size_t pos = 0;
        std::string token;
        while ((pos = line.find(delimiter)) != std::string::npos) {
            token = line.substr(0, pos);
            images.push_back(token);
            line.erase(0, pos + delimiter.length());
        }
        images.push_back(line);

        if (images.size() < TRAX_CHANNELS) {
            images.resize(TRAX_CHANNELS);
        }

        list.push_back(images);
    }

    input.close();
}

void save_timings(const string& file, vector<double>& timings) {

    std::ofstream output;

    output.open(file.c_str(), std::ofstream::out);

    for (vector<double>::iterator it = timings.begin(); it != timings.end(); it++) {

        output << *it << endl;

    }

    output.close();

}

Image load_image(string& path, int formats) {

    int image_format = 0;

    if TRAX_SUPPORTS(formats, TRAX_IMAGE_PATH)
        image_format = TRAX_IMAGE_PATH;
    else if TRAX_SUPPORTS(formats, TRAX_IMAGE_BUFFER)
        image_format = TRAX_IMAGE_BUFFER;
#ifdef TRAX_BUILD_OPENCV
    else if TRAX_SUPPORTS(formats, TRAX_IMAGE_MEMORY)
        image_format = TRAX_IMAGE_MEMORY;
#endif
    else
        throw std::runtime_error("No supported image format allowed");

    Image image;

    if (path.empty())
        return image;

    switch (image_format) {
    case TRAX_IMAGE_PATH: {
        return Image::create_path(path);
    }
    case TRAX_IMAGE_BUFFER: {
        // Read the file to memory
        std::ifstream t(path.c_str());
        t.seekg(0, std::ios::end);
        size_t size = t.tellg();
        std::string buffer(size, ' ');
        t.seekg(0);
        t.read(&buffer[0], size);
        return Image::create_buffer(buffer.size(), buffer.c_str());
        break;
    }
#ifdef TRAX_BUILD_OPENCV
    case TRAX_IMAGE_MEMORY: {
        cv::Mat cvmat = cv::imread(path);
        image = trax::mat_to_image(cvmat);
        break;
    }
#endif
    }

    return image;

}

ImageList load_images(vector<string>& path, int channels, int formats) {

    ImageList list;

    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_COLOR)) {
        list.set(load_image(path[0], formats), TRAX_CHANNEL_COLOR);
    }

    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_DEPTH)) {
        list.set(load_image(path[1], formats), TRAX_CHANNEL_DEPTH);
    }

    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_IR)) {
        list.set(load_image(path[2], formats), TRAX_CHANNEL_IR);
    }

    return list;

}

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

int main( int argc, char** argv) {
    int c;
    int result = 0;
    bool overlap_bounded = false;
    bool query_mode = false;
    opterr = 0;
    float threshold = -1;
    int timeout = 30;
    int reinitialize = 0;

    string timing_file;
    string tracker_command;
    string images_file("images.txt");
    string groundtruth_file("groundtruth.txt");
    string output_file("output.txt");
    string initialization_file;

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
            case 's':
                verbosity = VERBOSITY_SILENT;
                break;
            case 'x':
                connection = CONNECTION_EXPLICIT;
                break;
            case 'X':
                connection = CONNECTION_SOCKETS;
                break;
            case 'Q':
                query_mode = true;
                break;
            case 'I':
                images_file = string(optarg);
                break;
            case 'G':
                groundtruth_file = string(optarg);
                break;
            case 'O':
                output_file = string(optarg);
                break;
            case 'S':
                initialization_file = string(optarg);
                break;
            case 'f':
                threshold = (float) MIN(1, MAX(0, (float)atof(optarg)));
                break;
            case 'r':
                reinitialize = MAX(1, atoi(optarg));
                break;
            case 't':
                timeout = MAX(0, atoi(optarg));
                break;
            case 'T':
                timing_file = string(optarg);
                break;
            case 'e': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                environment[string(var, ptr-var)] = string(ptr+1);
                break;
            }
            case 'p': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                string key(var, ptr-var);
                string value(ptr+1);
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

        if(getenv("TRAX_BOUNDED_OVERLAP")) {
            printf("TRAX_BOUNDED_OVERLAP: %s \n", getenv("TRAX_BOUNDED_OVERLAP"));
            overlap_bounded = strcmpi(getenv("TRAX_BOUNDED_OVERLAP"), "true") == 0;
            if (overlap_bounded)
                print_debug("Using bounded region overlap calculation\n");
        }

        if(getenv("TRAX_REGION_LEGACY")) {
            printf("TRAX_REGION_LEGACY: %s \n", getenv("TRAX_REGION_LEGACY"));
            if (strcmpi(getenv("TRAX_REGION_LEGACY"), "true") == 0)
                region_set_flags(REGION_LEGACY_RASTERIZATION);
        }

        print_debug("Tracker command: '%s'\n", tracker_command.c_str());

        if (threshold >= 0)
            print_debug("Failure overlap threshold: %.2f\n", threshold);

        if (timeout >= 1)
            print_debug("Timeout: %d\n", timeout);

        if (query_mode) {
            // Query mode: in query mode we only check if the client successfuly connects to the tracker and receives introduction message.
            // In the future we can also output some basic data about the tracker.

            TrackerProcess tracker(tracker_command, environment, timeout, connection, verbosity);

            tracker.register_watchdog(must_exit);

            if (tracker.query()) {

                Metadata metadata = tracker.metadata();

                cout << "Tracker name: " << metadata.tracker_name() << endl;
                cout << "Tracker description: " << metadata.tracker_description() << endl;
                cout << "Tracker family: " << metadata.tracker_family() << endl;

                result = 0;
            } else {
                result = -1;
            }

        } else {

            // Tracking mode: the default mode where the entire sequence is processed.

            vector<vector<string> > images;
            vector<Region> groundtruth;
            vector<Region> initialization;
            vector<Region> output;
            vector<double> timings;

            read_images(images_file, images);
            load_trajectory(groundtruth_file, groundtruth);

            print_debug("Images loaded from file %s.\n", images_file.c_str());
            print_debug("Groundtruth loaded from file %s.\n", groundtruth_file.c_str());

            if (images.size() < groundtruth.size()) {
                print_debug("Warning: Image sequence shorter that groundtruth. Truncating.\n");
                groundtruth = vector<Region>(groundtruth.begin(), groundtruth.begin() + images.size());
            }

            if (images.size() > groundtruth.size()) {
                print_debug("Warning: Image sequence longer that groundtruth. Truncating.\n");
                images = vector<vector<string> >(images.begin(), images.begin() + groundtruth.size());
            }

            if (!initialization_file.empty()) {
                load_trajectory(initialization_file, initialization);
                print_debug("Initialization loaded from file %s.\n", initialization_file.c_str());
            } else {
                initialization = vector<Region>(groundtruth.begin(), groundtruth.end());
            }

            print_debug("Sequence length: %d frames.\n", (int) images.size());

            TrackerProcess tracker(tracker_command, environment, timeout, connection, verbosity);

            tracker.register_watchdog(must_exit);

            tracker.query();

            size_t frame = 0;
            while (frame < images.size()) {

                for (; frame < images.size(); frame++) {
                    if (!initialization[frame].empty()) break;
                    print_debug("Skipping frame %d, no initialization data. \n", (int) frame);
                    output.push_back(Region::create_special(0));
                }

                if (frame == images.size()) break;

                if (!tracker.ready()) {
                    throw std::runtime_error("Tracker process not alive anymore.");
                }

                print_debug("Loading initialization images.\n");

                Metadata metadata = tracker.metadata();

                Region initialize = initialization[frame];
                ImageList image = load_images(images[frame], metadata.image_formats(), metadata.channels());

                // Start timing a frame
                double timing_elapsed;
                timer_state timing_start = timer_clock();

                if (!tracker.initialize(image, initialize, properties)) {
                    throw std::runtime_error("Unable to initialize tracker.");
                }

                bool initialized = true;

                while (true) {
                    // Repeat while tracking the target.

                    Region status;
                    Properties additional;

                    bool result = tracker.wait(status, additional);

                    // Stop timing a frame
                    timing_elapsed = timer_elapsed(timing_start);

                    if (result) {
                        // Default option, the tracker returns a valid status.

                        Region reference = groundtruth[frame];
                        Bounds bounds;

                        if (overlap_bounded) {
                            image_size is = image_get_size(image);
                            print_debug("Bounds for overlap calculation %dx%d\n", is.width, is.height);
                            bounds = Bounds(0, 0, is.width, is.height);
                        }

                        float overlap = reference.overlap(status, bounds);

                        print_debug("Region overlap: %.2f\n", overlap);

                        // Check the failure criterion.
                        if (threshold >= 0 && overlap <= threshold) {
                            // Break the tracking loop if the tracker failed.
                            break;
                        }

                        if (!initialized)
                            output.push_back(status);
                        else {
                            output.push_back(Region::create_special(1));
                            initialized = false;
                        }

                        timings.push_back(timing_elapsed);

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

                    frame++;

                    if (frame >= images.size()) break;

                    print_debug("Loading frame images.\n");
                    ImageList image = load_images(images[frame], metadata.image_formats(), metadata.channels());

                    // Start timing a frame
                    timing_start = timer_clock();

                    Properties no_properties;
                    if (!tracker.frame(image, no_properties))
                        throw std::runtime_error("Unable to send new frame.");

                }

                if (frame < images.size()) {
                    // If the tracker was not successful and we have to consider the remaining frames.

                    if (reinitialize > 0) {
                        // If reinitialization is specified after 1 or more frames ...
                        size_t j = frame + 1;
                        output.push_back(Region::create_special(2));
                        timings.push_back(0);
                        for (; j < frame + reinitialize && j < images.size(); j++) {
                            output.push_back(Region::create_special(0));
                            timings.push_back(0);
                        }
                        frame = j;

                        if (frame < images.size()) {
                            tracker.reset();
                            tracker.query();
                        }

                    } else {
                        // ... otherwise just fill the remaining part of sequence with empty frames.
                        size_t j = frame + 1;
                        output.push_back(Region::create_special(2));
                        timings.push_back(0);
                        for (; j < images.size(); j++) {
                            output.push_back(Region::create_special(0));
                            timings.push_back(0);
                        }
                        break;
                    }
                } else {

                }
            }

            if (output.size() > 0)
                save_trajectory(output_file, output);

            if (timings.size() > 0 && timing_file.size() > 0)
                save_timings(timing_file, timings);

        }

    } catch (const std::runtime_error &e) {

        fprintf(stderr, "Error: %s\n", e.what());
        result = -1;

    }

    exit(result);
}

