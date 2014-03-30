/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a stationary tracker. It only reports the initial 
 * position for all frames and is used for testing purposes.
 * The main function of this example is to show the developers how to modify
 * their trackers to work with the evaluation environment.
 *
 * Copyright (c) 2013, Luka Cehovin
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

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdexcept>
#include <iostream>
#include <map>

#include "trax.h"
#include "region.h"

#include "process.h"
#include "threads.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include "getopt_win.h"
inline void sleep(long time) {
	Sleep(time);
}
#endif

using namespace std;

#define CMD_OPTIONS "hdI:G:f:O:r:t:T:p:e:"

#define DEBUGMSG(...) if (debug) { fprintf(stdout, "CLIENT: "); fprintf(stdout, __VA_ARGS__); }

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

typedef struct watchdog_state {
    bool active;
    int counter;
    Process* process;
} watchdog_state;


float threshold = -1;
int timeout = 30;
bool debug = false;
int reinitialize = 0;
string imagesFile;
string groundtruthFile;
string outputFile;
string timingFile;
string trackerCommand;

THREAD watchdog;
THREAD_MUTEX watchdogMutex;
watchdog_state watchdogState;

void print_help() {

    cout << "TraX Client" << "\n\n";
    cout << "Built on " << __DATE__ << " at " << __TIME__ << "\n";
    cout << "Protocol version: " << TRAX_VERSION << "\n\n";

    cout << "Usage: traxclient [-h] [-d] [-I image_list] [-O output_file] \n";
    cout << "\t [-f threshold] [-r frames] [-G groundtruth_file] [-e name=value] \n";
    cout << "\t [-p name=value] [-t timeout]  [-T timings_file]\n";
    cout << "\t <command_part1> <command_part2> ...";

    cout << "\n\nProgram arguments: \n";
    cout << "\t-h\tPrint this help and exit\n";
    cout << "\t-d\tEnable debug\n";
    cout << "\t-T\tGather time data\n";
    cout << "\t-t\tSpecify wait timeout\n";
    cout << "\t-G\tGroundtruth annotations file\n";
    cout << "\t-I\tFile that lists the image sequence files\n";
    cout << "\t-O\tOutput region file\n";
    cout << "\t-T\tOutput timings file\n";
    cout << "\t-f\tFailure threshold\n";
    cout << "\t-r\tReinitialization offset\n";
    cout << "\t-e\tEnvironmental variable (multiple occurences allowed)\n";
    cout << "\t-p\tTracker parameter (multiple occurences allowed)\n";
    cout << "\n";

    cout << "\n";
}

float compute_overlap(trax_region* ra, trax_region* rb) {

    int type = MIN(ra->type, rb->type);

	float iLeft = MAX(ra->data.rectangle.x, rb->data.rectangle.x);
	float iTop = MAX(ra->data.rectangle.y, rb->data.rectangle.y);
	float iRight = MIN(ra->data.rectangle.x + ra->data.rectangle.width, 
            rb->data.rectangle.x + rb->data.rectangle.width);
	float iBottom = MIN(ra->data.rectangle.y + ra->data.rectangle.height,
            rb->data.rectangle.y + rb->data.rectangle.height);

	if (iLeft > iRight || iTop > iBottom) return true;

	float intersectArea = (iRight - iLeft) * (iBottom - iTop);
	float unionArea = (ra->data.rectangle.width * ra->data.rectangle.height) +
		  (rb->data.rectangle.width * rb->data.rectangle.height) - intersectArea;

	return (intersectArea / unionArea);

}

void load_data(vector<trax_image*>& images, vector<trax_region*>& groundtruth) {

    FILE *imgFp = fopen(imagesFile.c_str(), "r");
    FILE *gtFp = fopen(groundtruthFile.c_str(), "r");

    if (!gtFp) {
        throw std::runtime_error("Groundtruth file not available.");
    }

    if (!imgFp) {
        throw std::runtime_error("Image list file  not available.");
    }

    size_t linesiz = sizeof(char) * 2048;
    char* gt_linebuf = (char*) malloc(linesiz);
    char* img_linebuf = (char*) malloc(linesiz);
    ssize_t gt_linelen = 0;
    ssize_t img_linelen = 0;
    int x, y, width, height;

    while (1) {

        if ((gt_linelen=getline(&gt_linebuf, &linesiz, gtFp))<1)
            break;

        if ((img_linelen=getline(&img_linebuf, &linesiz, imgFp))<1)
            break;

        if ((gt_linebuf)[gt_linelen - 1] == '\n') { (gt_linebuf)[gt_linelen - 1] = '\0'; }
        if ((img_linebuf)[img_linelen - 1] == '\n') { (img_linebuf)[img_linelen - 1] = '\0'; }

        trax_region* region;

        if (parse_region(gt_linebuf, &region))
            groundtruth.push_back(region);
        else
            continue; // TODO: do not know how to handle this ... probably a warning

        images.push_back(trax_image_create_path(img_linebuf));

    }    
    
    fclose(gtFp);
    fclose(imgFp);

    free(gt_linebuf);
    free(img_linebuf);

}

void save_data(vector<trax_region*>& output) {

    FILE *outFp = fopen(outputFile.c_str(), "w");

    if (!outFp) {
        throw std::runtime_error("Unable to open output file for writing.");
    }

    for (int i = 0; i < output.size(); i++) {    

        trax_region* r = output[i];

        if (!r) {
            fprintf(outFp, "\n");
            continue;
        }

        print_region(outFp, r);
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

void* watchdog_loop(void* param) {

    bool run = true;

    while (run) {

        usleep(1000000);
        
        MUTEX_LOCK(watchdogMutex);

        run = watchdogState.active;

        if (watchdogState.counter > 0)
            watchdogState.counter--;
        else {
            if (watchdogState.process) {
                DEBUGMSG("Timeout reached. Stopping tracker process ...\n");
                watchdogState.process->stop();
                watchdogState.process = NULL;
            }
        }

        MUTEX_UNLOCK(watchdogMutex);

    }

}


int main( int argc, char** argv) {
    
    int result = 0;
    int c;
    debug = false;
    opterr = 0;
    imagesFile = string("images.txt");
    groundtruthFile = string("groundtruth.txt");
    outputFile = string("output.txt");

    trax_handle* trax = NULL;
    Process* trackerProcess = NULL;

    vector<trax_image*> images;
    vector<trax_region*> groundtruth;
    vector<trax_region*> output;
    vector<long> timings;
    map<string, string> environment;
    trax_properties* properties = trax_properties_create();

    THREAD watchdog_thread;

    try {

        while ((c = getopt(argc, argv, CMD_OPTIONS)) != -1)
            switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'd':
                debug = true;
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
            case 'f':
                threshold = MIN(1, MAX(0, atof(optarg)));
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

            trackerCommand = string("");

            for (int i = optind; i < argc; i++)
                trackerCommand += string(argv[optind]) + string(" ");

        } else {
            print_help();
            exit(-1);
        }

        DEBUGMSG("Tracker command: '%s'\n", trackerCommand.c_str());

        if (threshold >= 0)
            DEBUGMSG("Failure overlap threshold: %.2f\n", threshold);

        if (timeout >= 1)
            DEBUGMSG("Timeout: %d\n", timeout);

        load_data(images, groundtruth);

        watchdogState.process = NULL;
        watchdogState.active = true;
        watchdogState.counter = timeout;

        MUTEX_INIT(watchdogMutex);

        if (timeout > 0)
            CREATE_THREAD(&watchdog, watchdog_loop, NULL);

        int frame = 0;
        while (frame < images.size()) {

            trackerProcess = new Process(trackerCommand);

            trackerProcess->copy_environment();

            trackerProcess->start();

            DEBUGMSG("Tracker process ID: %d \n", trackerProcess->get_handle());

            int flags = debug ? (TRAX_FLAG_LOG_INPUT | TRAX_FLAG_LOG_OUTPUT) : 0;

            MUTEX_LOCK(watchdogMutex);

            watchdogState.counter = timeout;
            watchdogState.process = trackerProcess;

            MUTEX_UNLOCK(watchdogMutex);

            trax = trax_client_setup(trackerProcess->get_output(), trackerProcess->get_input(), stdout, flags);

            if (!trax) throw std::runtime_error("Unable to establish connection");

            if (trax->version > TRAX_VERSION) throw std::runtime_error("Unsupported protocol version");

            if (trax->config.format_region != TRAX_REGION_RECTANGLE)  throw std::runtime_error("Unsupported region format");

            if (trax->config.format_image != TRAX_IMAGE_PATH)  throw std::runtime_error("Unsupported image format");

            trax_image* image = images[frame];
            trax_region* initialize = groundtruth[frame];

            // Start timing
            clock_t timing_toc;
            clock_t timing_tic = clock();

            trax_client_initialize(trax, image, initialize, properties);

            while (true) {

                trax_region* status;

                trax_properties* additional = trax_properties_create();

                MUTEX_LOCK(watchdogMutex);

                watchdogState.counter = timeout;

                MUTEX_UNLOCK(watchdogMutex);

                int result = trax_client_wait(trax, &status, additional);

                // Stop timing
                timing_toc = clock();

                if (result == TRAX_STATUS) {

                    trax_region* gt = groundtruth[frame];

                    float overlap = compute_overlap(gt, status);

                    DEBUGMSG("Region overlap: %.2f\n", overlap);

                    if (threshold >= 0 && overlap <= threshold) {
                        break;
                    }
                   
                    trax_properties_release(&additional);

                    output.push_back(status);
                    timings.push_back(((timing_toc - timing_tic) * 1000) / CLOCKS_PER_SEC);

                } else if (result == TRAX_STATUS) {                    

                    trax_properties_release(&additional);
                    break;
                } else {
                    trax_properties_release(&additional);
                    throw std::runtime_error("Unable to contact tracker.");
                }

                frame++;

                if (frame >= images.size()) break;

                trax_image* image = images[frame];

                // Start timing
                timing_tic = clock();

                trax_client_frame(trax, image, NULL);

            }

            trax_cleanup(&trax);

            MUTEX_LOCK(watchdogMutex);

            watchdogState.process = NULL;

            MUTEX_UNLOCK(watchdogMutex);

            sleep(0.1);

            trackerProcess->stop();
            delete trackerProcess;
            trackerProcess = NULL;

            if (reinitialize > 0) {
                int j = frame;
                for (; j < frame + reinitialize && j < images.size(); j++) {
                    output.push_back(NULL);
                    timings.push_back(0);
                }
                frame = j;
            } else break;
        }


        if (output.size() > 0)
            save_data(output);

        if (timings.size() > 0 && timingFile.size() > 0)
            save_timings(timings);

    } catch (std::runtime_error e) {
        
        fprintf(stderr, "Error: %s\n", e.what());

        trax_cleanup(&trax);

        if (trackerProcess) {
            trackerProcess->stop();
            delete trackerProcess;
            trackerProcess = NULL;
        }

        result = -1;

    }

    MUTEX_LOCK(watchdogMutex);

    watchdogState.active = false;
    watchdogState.process = NULL;

    MUTEX_UNLOCK(watchdogMutex);

    MUTEX_DESTROY(watchdogMutex);

    trax_properties_release(&properties);

    // Cleanup ...
    for (int i = 0; i < images.size(); i++) {
        trax_image_release(&images[i]);
        trax_region_release(&groundtruth[i]);
        if (output.size() > i && output[i]) trax_region_release(&output[i]);

    }

    exit(result);
}

