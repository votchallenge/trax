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

#define _BSD_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <stdexcept>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <trax/client.hpp>

#ifdef TRAX_BUILD_OPENCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <trax/opencv.hpp>
#endif

#define TRAX_DEFAULT_PORT 9090

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>

#define strcmpi _strcmpi

__inline void sleep(long time) {
    Sleep(time * 1000);
}

#define __INLINE __inline

#else

#ifdef _MAC_
    
#else
    #include <unistd.h>
#endif

#include <signal.h>
#define strcmpi strcasecmp
#define __INLINE inline

#endif

#include "getopt.h"
#include "utilities.h"

using namespace std;
using namespace trax;

#define LOGGER_BUFFER_SIZE 1024

#define CMD_OPTIONS "hsdI:G:f:O:S:r:t:T:p:e:xXQ"

#define DEBUGMSG(...) if (debug) { fprintf(stdout, "CLIENT: "); fprintf(stdout, __VA_ARGS__); }

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

float threshold = -1;
int timeout = 30;
bool debug = false;
bool silent = false;
int reinitialize = 0;
string imagesFile;
string groundtruthFile;
string initializationFile;
string outputFile;
string timingFile;
string trackerCommand;

void print_help() {

    cout << "TraX Client" << "\n\n";
    cout << "Built on " << __DATE__ << " at " << __TIME__ << "\n";
    cout << "Library version: " << trax_version() << "\n";
    cout << "Protocol version: " << TRAX_VERSION << "\n\n";

    cout << "Usage: traxclient [-h] [-d] [-I image_list] [-O output_file] \n";
    cout << "\t [-f threshold] [-r frames] [-G groundtruth_file] [-e name=value] \n";
    cout << "\t [-p name=value] [-t timeout] [-s] [-T timings_file] [-x] [-X]\n";
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

void load_data(vector<string>& images, vector<region_container*>& groundtruth, vector<region_container*>& initialization) {

    std::ifstream if_images, if_groundtruth, if_initialization;

    if_images.open(imagesFile.c_str(), std::ifstream::in);
    if_groundtruth.open(groundtruthFile.c_str(), std::ifstream::in);

    if (!initializationFile.empty())
        if_initialization.open(initializationFile.c_str(), std::ifstream::in);

    if (!if_groundtruth.is_open()) {
        throw std::runtime_error("Groundtruth file not available.");
    }

    if (!if_images.is_open()) {
        throw std::runtime_error("Image list file not available.");
    }

    size_t line_size = sizeof(char) * 2048;
    char* gt_line_buffer = (char*) malloc(line_size);
    char* it_line_buffer = (char*) malloc(line_size);
    char* im_line_buffer = (char*) malloc(line_size);

    int line = 0;

    while (1) {

        if_groundtruth.getline(gt_line_buffer, line_size);

        if (!if_groundtruth.good()) break;

        if_images.getline(im_line_buffer, line_size);

        if (!if_images.good()) break;

        line++;

        region_container* region;

        if (region_parse(gt_line_buffer, &region)) {

            groundtruth.push_back(region);
            images.push_back(string(im_line_buffer));

        } else
            DEBUGMSG("Unable to parse region data at line %d!\n", line); // TODO: do not know how to handle this ... probably a warning

        if (if_initialization.is_open()) {

            if_initialization.getline(it_line_buffer, line_size);

            if (!if_initialization.good()) break;

            if (!if_initialization.good()) {

                initialization.push_back(NULL);

            } else {

                region_container* initialization_region;

                if (region_parse(it_line_buffer, &initialization_region)) {

                    initialization.push_back(initialization_region);

                } else initialization.push_back(NULL);

            }

        }

    }

    if_groundtruth.close();
    if_images.close();
    if (if_initialization.is_open()) if_initialization.close();

    free(gt_line_buffer);
    free(it_line_buffer);
    free(im_line_buffer);

}

void save_data(vector<region_container*>& output) {

    FILE *outFp = fopen(outputFile.c_str(), "w");

    if (!outFp) {
        throw std::runtime_error("Unable to open output file for writing.");
    }

    for (int i = 0; i < output.size(); i++) {

        region_container* r = output[i];

        if (!r) {
            fprintf(outFp, "\n");
            continue;
        }

        region_print(outFp, r);
        fprintf(outFp, "\n");

    }

    fclose(outFp);

}

void save_timings(vector<long>& timings) {

    FILE *outFp = fopen(timingFile.c_str(), "w");

    if (!outFp) {
        throw std::runtime_error("Unable to open timings file for writing.");
    }

    for (int i = 0; i < timings.size(); i++) {

       fprintf(outFp, "%ld\n", timings[i]);

    }

    fclose(outFp);

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

    Image image = NULL;

    switch (image_format) {
    case TRAX_IMAGE_PATH: {
        return Image.create_path(path);
    }
    case TRAX_IMAGE_BUFFER: {
        // Read the file to memory
        std::ifstream t(path.c_str());
        t.seekg(0, std::ios::end);
        size_t size = t.tellg();
        std::string buffer(size, ' ');
        t.seekg(0);
        t.read(&buffer[0], size); 
        return Image.create_buffer(buffer.size(), buffer.c_str());
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

int main( int argc, char** argv) {
    int c;
    int result = 0;
    bool overlap_bounded = false;
    bool query_mode = false;
    bool explicit_mode = false;
    bool socket_mode = false;
    debug = false;
    opterr = 0;
    imagesFile = string("images.txt");
    groundtruthFile = string("groundtruth.txt");
    outputFile = string("output.txt");
    initializationFile = string("");

    trax_handle* trax = NULL;
    Process* trackerProcess = NULL;

    vector<string> images;
    vector<region_container*> groundtruth;
    vector<region_container*> initialization;
    vector<region_container*> output;
    vector<long> timings;
    map<string, string> environment;
    trax_properties* properties = trax_properties_create();

    configure_signals();

    try {

        while ((c = getopt(argc, argv, CMD_OPTIONS)) != -1)
            switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'd':
                debug = true;
                break;
            case 's':
                silent = true;
                break;
            case 'x':
                explicit_mode = true;
                break;
            case 'X':
                socket_mode = true;
                break;
            case 'Q':
                query_mode = true;
                break;
            case 'I':
                imagesFile = string(optarg);
                break;
            case 'G':
                groundtruthFile = string(optarg);
                break;
            case 'O':
                outputFile = string(optarg);
                break;
            case 'S':
                initializationFile = string(optarg);
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
                timingFile = string(optarg);
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
                trax_properties_set(properties, key.c_str(), ptr+1);
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

            trackerCommand = buffer.str();


        } else {
            print_help();
            exit(-1);
        }

        if(getenv("TRAX_BOUNDED_OVERLAP")) {
            printf("TRAX_BOUNDED_OVERLAP: %s \n", getenv("TRAX_BOUNDED_OVERLAP"));
            overlap_bounded = strcmpi(getenv("TRAX_BOUNDED_OVERLAP"), "true") == 0;
            if (overlap_bounded)      
                DEBUGMSG("Using bounded region overlap calculation\n");
        }

        if(getenv("TRAX_REGION_LEGACY")) {
            printf("TRAX_REGION_LEGACY: %s \n", getenv("TRAX_REGION_LEGACY"));
            if (strcmpi(getenv("TRAX_REGION_LEGACY"), "true") == 0)
                region_set_flags(REGION_LEGACY_RASTERIZATION);
        }

        DEBUGMSG("Tracker command: '%s'\n", trackerCommand.c_str());

        if (threshold >= 0)
            DEBUGMSG("Failure overlap threshold: %.2f\n", threshold);

        if (timeout >= 1)
            DEBUGMSG("Timeout: %d\n", timeout);

        if (!query_mode)
            load_data(images, groundtruth, initialization);

        if (query_mode) {
        // Query mode: in query mode we only check if the client successfuly connects to the tracker and receives introduction message.
        // In the future we can also output some basic data about the tracker.

            TrackerProcess tracker(trackerCommand, environment);
            if (tracker.test()) {
                result = 0;
            } else {
                result = -1;
            }

        } else {
        // Tracking mode: the default mode where the entire sequence is processed.
            int frame = 0;
            while (frame < images.size()) {


                Image image = load_image(trax, images[frame]);
                
                trax_region* initialize = NULL;

                // Check if we can initialize from separate initialization source
                // or from groundtruth.
                if (initialization.size() > 0) {

                    for (; frame < images.size(); frame++) {
                        if (initialization[frame]) break;
                        output.push_back(region_create_special(0));
                    }

                    if (frame == images.size()) break;

                    initialize = initialization[frame];

                } else {

                    initialize = groundtruth[frame];

                }

                // Start timing a frame
                clock_t timing_toc;
                clock_t timing_tic = clock();

                while (true) {
                    // Repeat while tracking the target.

                    region_container* status = NULL;

                    trax_properties* additional = trax_properties_create();

                    watchdog_reset(trackerProcess, timeout);

                    int result = trax_client_wait(trax, (trax_region**) &status, additional);

                    // Stop timing a frame
                    timing_toc = clock();

                    if (result == TRAX_STATE) {
                        // Default option, the tracker returns a valid status.

                        region_container* gt = groundtruth[frame];
                        region_bounds bounds = region_no_bounds;

                        if (overlap_bounded) {
                            image_size is = image_get_size(image);
                            DEBUGMSG("Bounds for overlap calculation %dx%d\n", is.width, is.height); 
                            bounds = region_create_bounds(0, 0, is.width, is.height);
                        }

                        float overlap = region_compute_overlap(gt, status, bounds).overlap;

                        DEBUGMSG("Region overlap: %.2f\n", overlap);

                        trax_properties_release(&additional);

                        // Check the failure criterion.
                        if (threshold >= 0 && overlap <= threshold) {
                            // Break the tracking loop if the tracker failed.
                            break;
                        }

                        if (tracking)
                            output.push_back(status);
                        else {
                            region_release(&status);
                            output.push_back(region_create_special(1));
                        }

                        timings.push_back(((timing_toc - timing_tic) * 1000) / CLOCKS_PER_SEC);

                    } else if (result == TRAX_QUIT) {
                        // The tracker has requested termination of connection.

                        trax_properties_release(&additional);

                        DEBUGMSG("Termination requested by tracker.\n");

                        break;
                    } else {
                        // In case of an error ...

                        trax_properties_release(&additional);
                        throw std::runtime_error("Unable to contact tracker.");
                    }

                    frame++;

                    if (frame >= images.size()) break;

                    trax_image* image = load_image(trax, images[frame]);

                    // Start timing a frame
                    timing_tic = clock();

                    if (trax_client_frame(trax, image, NULL) < 0)
                        throw std::runtime_error("Unable to send new frame.");

                    trax_image_release(&image);

                    tracking = true;

                }

                trax_cleanup(&trax);

                watchdog_reset(NULL, 0);

                sleep(0);

                if (trackerProcess) {
                    trackerProcess->stop();
                    delete trackerProcess;
                    trackerProcess = NULL;
                }

                if (frame < images.size()) {
                    // If the tracker was not successful and we have to consider the remaining frames.

                    if (reinitialize > 0) {
                        // If reinitialization is specified after 1 or more frames ...
                        int j = frame + 1;
                        output.push_back(region_create_special(2));
                        timings.push_back(0);
                        for (; j < frame + reinitialize && j < images.size(); j++) {
                            output.push_back(region_create_special(0));
                            timings.push_back(0);
                        }
                        frame = j;
                    } else {
                        // ... otherwise just fill the remaining part of sequence with empty frames.
                        int j = frame + 1;
                        output.push_back(region_create_special(2));
                        timings.push_back(0);
                        for (; j < images.size(); j++) {
                            output.push_back(region_create_special(0));
                            timings.push_back(0);
                        }
                        break;
                    }
                }
            }

            if (output.size() > 0)
                save_data(output);

            if (timings.size() > 0 && timingFile.size() > 0)
                save_timings(timings);

        }

    } catch (std::runtime_error e) {

        fprintf(stderr, "Error: %s\n", e.what());

        trax_cleanup(&trax);

        if (trackerProcess) {
            int exit_status;

            trackerProcess->stop();
            trackerProcess->is_alive(&exit_status);

            delete trackerProcess;
            trackerProcess = NULL;

            if (exit_status == 0) {
                DEBUGMSG("Tracker exited normally.\n");
            } else {
                fprintf(stderr, "Tracker exited (exit code %d)\n", exit_status);
            }

        }

        result = -1;

    }

    trax_properties_release(&properties);

    for (int i = 0; i < images.size(); i++) {
        //trax_image_release(&images[i]);
        region_release(&groundtruth[i]);
        if (output.size() > i && output[i]) region_release(&output[i]);
        if (initialization.size() > i && initialization[i]) region_release(&initialization[i]);
    }

    exit(result);
}

