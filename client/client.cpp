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

#define TRAX_DEFAULT_PORT 9090

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>
#include "getopt_win.h"

#define strcmpi _strcmpi

static int initialized = 0;
static void initialize_sockets(void) {
    WSADATA data;
    WORD version = 0x101;
	if (initialized) return;

    WSAStartup(version,&data);
    initialized = 1;
    return;
}

__inline void sleep(long time) {
	Sleep(time * 1000);
}

#define __INLINE __inline

#else

#ifdef _MAC_
    #include <tcpd.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <sys/select.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #define closesocket close
#endif

#define strcmpi strcasecmp

#define __INLINE inline

static void initialize_sockets(void) {}

#endif

#include "trax.h"
#include "process.h"
#include "region.h"
#include "threads.h"

using namespace std;

#define CMD_OPTIONS "hsdI:G:f:O:S:r:t:T:p:e:xXQ"

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
bool silent = false;
int reinitialize = 0;
string imagesFile;
string groundtruthFile;
string initializationFile;
string outputFile;
string timingFile;
string trackerCommand;

THREAD watchdog;
THREAD_MUTEX watchdogMutex;
watchdog_state watchdogState;

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
    cout << "\t-T\tGather time data\n";
    cout << "\t-t\tSpecify wait timeout\n";
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

int create_server_socket(int port) {

	int sid;
	int one = 1;
	struct sockaddr_in sin;

	const char* hostname = TRAX_LOCALHOST;

	initialize_sockets();
 
	if((sid = (int) socket(AF_INET,SOCK_STREAM,0)) == -1) {
		return -1;
	}
	setsockopt(sid,SOL_SOCKET,SO_REUSEADDR,
						 (const char *)&one,sizeof(int));

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(hostname);

    sin.sin_port = htons(port);

    if(bind(sid,(struct sockaddr *)&sin,sizeof(sin)) == -1) {
        perror("bind");
        closesocket(sid);
        return -1;
    }

	if(listen(sid, 1)== -1) {
		perror("listen");
		closesocket(sid);
		return -1;
	}

    return sid;

}

void destroy_server_socket(int server) {

    if (server < 0) return;

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
    shutdown(server, SD_BOTH);
#else
    shutdown(server, SHUT_RDWR);
#endif
    closesocket(server);

}

void load_data(vector<trax_image*>& images, vector<region_container*>& groundtruth, vector<region_container*>& initialization) {

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
            images.push_back(trax_image_create_path(im_line_buffer));

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

THREAD_CALLBACK(watchdog_loop, param) {

    bool run = true;

    while (run) {

        sleep(1);
        
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

    return 0;

}

void watchdog_reset(Process* process, int counter) {

    MUTEX_LOCK(watchdogMutex);

    watchdogState.active = counter > 0;
    watchdogState.counter = counter;
    watchdogState.process = process;

    MUTEX_UNLOCK(watchdogMutex);

}

Process* configure_process(const string& command, bool explicitMode, const map<string, string>& environment) {

    Process* process = new Process(trackerCommand, explicitMode);

    process->copy_environment();
    
    std::map<std::string, std::string>::const_iterator iter;
    for (iter = environment.begin(); iter != environment.end(); ++iter) {
       process->set_environment(iter->first, iter->second);
    }

    return process;
}

int main( int argc, char** argv) {
    
    int result = 0;
    int c;
    bool query_mode = false;
    bool explicit_mode = false;
    bool socket_mode = false;
    int socket_id = -1;
    int socket_port = TRAX_DEFAULT_PORT;
    debug = false;
    opterr = 0;
    imagesFile = string("images.txt");
    groundtruthFile = string("groundtruth.txt");
    outputFile = string("output.txt");
    initializationFile = string("");

    trax_handle* trax = NULL;
    Process* trackerProcess = NULL;

    vector<trax_image*> images;
    vector<region_container*> groundtruth;
    vector<region_container*> initialization;
    vector<region_container*> output;
    vector<long> timings;
    map<string, string> environment;
    trax_properties* properties = trax_properties_create();

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

        DEBUGMSG("Tracker command: '%s'\n", trackerCommand.c_str());

        if (threshold >= 0)
            DEBUGMSG("Failure overlap threshold: %.2f\n", threshold);

        if (timeout >= 1)
            DEBUGMSG("Timeout: %d\n", timeout);

        if (socket_mode) {
            // Try to create a listening socket by looking for a free port number.
            while (true) {
                socket_id = create_server_socket(socket_port);
                if (socket_id < 0) {
                    socket_port++;
                    if (socket_port > 65535) throw std::runtime_error("Unable to configure TCP server socket.");
                } else break;
            }
            
            DEBUGMSG("Socket opened successfully on port %d.\n", socket_port);

        }

        if (!query_mode)
            load_data(images, groundtruth, initialization);

        watchdogState.process = NULL;
        watchdogState.active = true;
        watchdogState.counter = timeout;

        MUTEX_INIT(watchdogMutex);

        if (timeout > 0)
            CREATE_THREAD(watchdog, watchdog_loop, NULL);

        if (query_mode) {
            // In query mode we just start the process and wait for the introduction of the tracker ...

            trackerProcess = configure_process(trackerCommand, explicit_mode, environment);

            watchdog_reset(trackerProcess, timeout);

            if (socket_mode) {
                char port_buffer[24];
                sprintf(port_buffer, "%d", socket_port);
                trackerProcess->set_environment("TRAX_SOCKET", port_buffer);
            } 

            if (!trackerProcess->start()) {
		        DEBUGMSG("Unable to start the tracker process\n");
		        result = -2;
	        } else {

                if (socket_mode) {
                    trax = trax_client_setup_socket(socket_id, silent ? NULL : stdout);
                } else {
                    trax = trax_client_setup_file(trackerProcess->get_output(), trackerProcess->get_input(), silent ? NULL : stdout);
                }

                if (!trax) throw std::runtime_error("Unable to establish connection.");

                trax_cleanup(&trax);

                if (trackerProcess) {
                    trackerProcess->stop();
                    delete trackerProcess;
                    trackerProcess = NULL;
                }

                result = 0;

            }

        } else {

            int frame = 0;
            while (frame < images.size()) {

                trackerProcess = configure_process(trackerCommand, explicit_mode, environment);

                watchdog_reset(trackerProcess, timeout);

                if (socket_mode) {
                    char port_buffer[24];
                    sprintf(port_buffer, "%d", socket_port);
                    trackerProcess->set_environment("TRAX_SOCKET", port_buffer);
                } 

                if (!trackerProcess->start()) {
			        DEBUGMSG("Unable to start the tracker process\n");
			        break;
		        }

                if (socket_mode) {
                    trax = trax_client_setup_socket(socket_id, silent ? NULL : stdout);
                } else {
                    trax = trax_client_setup_file(trackerProcess->get_output(), trackerProcess->get_input(), silent ? NULL : stdout);
                }

                if (!trax) throw std::runtime_error("Unable to establish connection.");

                DEBUGMSG("Tracker process ID: %d \n", trackerProcess->get_handle());

                if (trax->version > TRAX_VERSION) throw std::runtime_error("Unsupported protocol version");

                if (trax->config.format_image != TRAX_IMAGE_PATH)  throw std::runtime_error("Unsupported image format");

                trax_image* image = images[frame];
                trax_region* initialize = NULL;

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
	
                bool tracking = false;

                trax_client_initialize(trax, image, initialize, properties);

                while (true) {

                    region_container* status = NULL;

                    trax_properties* additional = trax_properties_create();

                    watchdog_reset(trackerProcess, timeout);

                    int result = trax_client_wait(trax, (trax_region**) &status, additional);

                    // Stop timing a frame
                    timing_toc = clock();

                    if (result == TRAX_STATUS) {

                        region_container* gt = groundtruth[frame];

                        float overlap = region_compute_overlap(gt, status).overlap;

                        DEBUGMSG("Region overlap: %.2f\n", overlap);

                        trax_properties_release(&additional);

                        if (threshold >= 0 && overlap <= threshold) {
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

                        trax_properties_release(&additional);

                        DEBUGMSG("Termination requested by tracker.\n");

                        break;
                    } else {
                        trax_properties_release(&additional);
                        throw std::runtime_error("Unable to contact tracker.");
                    }

                    frame++;

                    if (frame >= images.size()) break;

                    trax_image* image = images[frame];

                    // Start timing a frame
                    timing_tic = clock();

                    trax_client_frame(trax, image, NULL);

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

                if (reinitialize > 0) {
                    int j = frame;
                    for (; j < frame + reinitialize && j < images.size(); j++) {
                        output.push_back(region_create_special(j == frame ? 2 : 0));
                        timings.push_back(0);
                    }
                    frame = j;
                } else break;
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
            trackerProcess->stop();
            delete trackerProcess;
            trackerProcess = NULL;
        }

        result = -1;

    }

    watchdog_reset(NULL, -1);

    DEBUGMSG("Cleaning up.\n");

    if (socket_mode) {
        destroy_server_socket(socket_id);
    }

    MUTEX_DESTROY(watchdogMutex);

    trax_properties_release(&properties);

    for (int i = 0; i < images.size(); i++) {
        trax_image_release(&images[i]);
        region_release(&groundtruth[i]);
        if (output.size() > i && output[i]) region_release(&output[i]);
        if (initialization.size() > i && initialization[i]) region_release(&initialization[i]);
    }

    RELEASE_THREAD(watchdog); 

    exit(result);
}

