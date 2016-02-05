/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Stripped down version of client.cpp. Test for Python server
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

#include <signal.h>

#define strcmpi strcasecmp

#define __INLINE inline

static void initialize_sockets(void) {}

#endif

#include "trax.h"
#include "message.h"
#include "process.h"
#include "region.h"
#include "threads.h"

using namespace std;

#define LOGGER_BUFFER_SIZE 1024

#define CMD_OPTIONS "hsdI:G:f:O:S:r:t:T:p:e:xXQ"

#define DEBUGMSG(...) if (true) { fprintf(stdout, "CLIENT: "); fprintf(stdout, __VA_ARGS__); }

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


void configure_signals() {

    #if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 

    // TODO: anything to do here?

    #else

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
      perror(0);
      exit(1);
    }

    #endif

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

int main( int argc, char** argv) {


    int result = 0;
    int c;
    bool socket_mode = true;
    int socket_id = -1;
    int socket_port = TRAX_DEFAULT_PORT;
    debug = false;
    opterr = 0;
    imagesFile = string("images.txt");
    groundtruthFile = string("groundtruth.txt");
    outputFile = string("output.txt");
    initializationFile = string("");

    trax_handle* trax = NULL;

    vector<trax_image*> images;
    vector<region_container*> groundtruth;
    vector<region_container*> initialization;
    vector<region_container*> output;
    trax_properties* properties = trax_properties_create();

    configure_signals();

    try {

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


        load_data(images, groundtruth, initialization);


        char port_buffer[24];
        sprintf(port_buffer, "%d", socket_port);


        DEBUGMSG("Setting up TraX in socket mode here\n");
        trax = trax_client_setup_socket(socket_id, timeout, NULL);
        int frame = 0;
        trax_image* image = images[frame];
        trax_region* initialize = groundtruth[frame];
				
		if (!trax) throw std::runtime_error("Unable to establish connection.");

        trax_client_initialize(trax, image, initialize, properties);

        frame += 1;
        if (!trax)
            throw std::runtime_error("Unable to establish connection.");

        while (frame < images.size()) {
            region_container* status = NULL;
            trax_properties* additional = trax_properties_create();

            int result = trax_client_wait(trax, (trax_region**) &status, additional);
            DEBUGMSG("Tracked region received. status %d \n",result);
            if (result == TRAX_STATUS) {
                DEBUGMSG("Tracked region received\n");
            }

            trax_image* image = images[frame];
            trax_client_frame(trax, image, NULL);

            frame += 1;
        }

        trax_cleanup(&trax);

        result = 0;

    } catch (std::runtime_error e) {
        DEBUGMSG("Error: %s\n", e.what());
        fprintf(stderr, "Error: %s\n", e.what());

        trax_cleanup(&trax);

    }

    DEBUGMSG("Cleaning up.\n");

    destroy_server_socket(socket_id);

    trax_properties_release(&properties);

    for (int i = 0; i < images.size(); i++) {
        trax_image_release(&images[i]);
        region_release(&groundtruth[i]);
        if (output.size() > i && output[i]) region_release(&output[i]);
        if (initialization.size() > i && initialization[i]) region_release(&initialization[i]);
    }

    exit(result);
}

