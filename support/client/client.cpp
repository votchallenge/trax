
#include <iostream>
#include <fstream>
#include <sstream>

#include <trax/client.hpp>

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

#include "process.h"
#include "threads.h"

#define DEBUGMSG(state, ...) if (state->verbosity == VERBOSITY_DEBUG) { fprintf(stdout, "CLIENT: "); fprintf(stdout, __VA_ARGS__); }

#define LOGGER_BUFFER_SIZE 1024

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

int read_stream(int fd, char* buffer, int len) {

    fd_set readfds,writefds,exceptfds;
    struct timeval tv;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (fd < 0 || fd >= FD_SETSIZE)
        return -1;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(fd,&readfds);
    FD_SET(fd,&exceptfds);

    if (select(fd+1, &readfds, &writefds, &exceptfds, &tv) < 0)
        return -1;

    if(FD_ISSET(fd,&readfds)) {
        return read(fd, buffer, len);
    } else if(FD_ISSET(fd,&exceptfds)) {
        return -1;
    } else return 0;

}

namespace trax {

class TrackerProcess::State {
public:
	State(const string& command, map<std::string, std::string> environment, ConnectionMode connection, VerbosityMode verbosity, int timeout):
		command(command), environment(environment), socket_id(-1), socket_port(TRAX_DEFAULT_PORT),
		connection(connection), verbosity(verbosity), timeout(timeout) {

        if (connection == CONNECTION_SOCKETS) {
            // Try to create a listening socket by looking for a free port number.
            while (true) {
                socket_id = create_server_socket(socket_port);
                if (socket_id < 0) {
                    socket_port++;
                    if (socket_port > 65535) throw std::runtime_error("Unable to configure TCP server socket.");
                } else break;
            }

            DEBUGMSG(this, "Socket opened successfully on port %d.\n", socket_port);

        }

        MUTEX_INIT(watchdog_mutex);
        MUTEX_INIT(logger_mutex);

        client = NULL;
        process = NULL;
        watchdog_active = true;
        logger_active = true;

        stop_watchdog();
        reset_logger();

        if (timeout > 0)
            CREATE_THREAD(watchdog_thread, watchdog_loop, this);

        CREATE_THREAD(logger_thread, logger_loop, this);

        start_process();

	}

	~State() {

		watchdog_active = false;
		logger_active = false;

		stop_process();

	    stop_watchdog();
	    reset_logger();

	    DEBUGMSG(this, "Cleaning up.\n");

	    if (connection == CONNECTION_SOCKETS) {
	        destroy_server_socket(socket_id);
	    }

	    RELEASE_THREAD(watchdog_thread);
	    RELEASE_THREAD(logger_thread);

	    MUTEX_DESTROY(watchdog_mutex);
	    MUTEX_DESTROY(logger_mutex);

	}

	bool start_process() {

		if (process) return false;

		DEBUGMSG(this, "Starting process %s\n", command.c_str());

	    process = new Process(command, connection == CONNECTION_EXPLICIT);
	    process->copy_environment();

	    std::map<std::string, std::string>::const_iterator iter;
	    for (iter = environment.begin(); iter != environment.end(); ++iter) {
	       process->set_environment(iter->first, iter->second);
	    }

	    if (connection == CONNECTION_SOCKETS) {
	        char port_buffer[24];
	        sprintf(port_buffer, "%d", socket_port);
	        process->set_environment("TRAX_SOCKET", port_buffer);
	    }

	    reset_logger();

	    if (!process->start()) {
	    	DEBUGMSG(this, "Unable to start process\n");
	        throw new std::runtime_error("Unable to start the tracker process");
	    } else {

	    	start_watchdog();

	    	Logging logger = trax_no_log;
			
			if (verbosity != VERBOSITY_SILENT) 
				logger = Logging(client_logger, this, 0);

	        if (connection == CONNECTION_SOCKETS) {
	            DEBUGMSG(this, "Setting up TraX with TCP socket connection\n");
	            client = new Client(socket_id, logger, timeout);
	        } else {
	            DEBUGMSG(this, "Setting up TraX with %s streams connection\n", (connection == CONNECTION_EXPLICIT) ? "dedicated" : "standard");
	            client = new Client(process->get_output(), process->get_input(), logger);
	        }
	        // TODO: check tracker exit state
	        if (!client) throw std::runtime_error("Unable to establish connection.");

			stop_watchdog();

		    int exit_status;
		    if (!process->is_alive(&exit_status)) {
		    	std::stringstream sstm;
				sstm << "Tracker exited with exit code " << exit_status;
		    	throw std::runtime_error(sstm.str());
		    }

		    DEBUGMSG(this, "Tracker process ID: %d \n", process->get_handle());

		    int server_version = 0;

		    client->get_parameter(TRAX_PARAMETER_VERSION, &server_version);

		    if (server_version > TRAX_VERSION) throw std::runtime_error("Unsupported protocol version");

		    DEBUGMSG(this, "Connection with tracker established.\n");

		    return true;

	    }

	}


	bool stop_process() {

		stop_watchdog();
		reset_logger();

        sleep(0);

		if (client) {
			delete client;
			client = NULL;
		}

        sleep(0);

		if (process) {
		    int exit_status;

		    process->stop();
		    process->is_alive(&exit_status);
		    process = NULL;

		    if (exit_status == 0) {
		        DEBUGMSG(this, "Tracker exited normally.\n");
		        return true;
		    } else {
		        fprintf(stderr, "Tracker exited (exit code %d)\n", exit_status);
		        return false;
		    }

		} 

		return true;

	}

	void reset_watchdog(int timeout) {

	    MUTEX_LOCK(watchdog_mutex);

	    watchdog_timeout = timeout;

	    MUTEX_UNLOCK(watchdog_mutex);

	}

	void start_watchdog() {

		reset_watchdog(timeout);

	}


	void stop_watchdog() {

		reset_watchdog(0);
		
	}

	void reset_logger() {

	    MUTEX_LOCK(logger_mutex);

	    stderr_position = 0;
	    stdout_position = 0;
	    stderr_length = 0;
	    stdout_length = 0;
	    logger_stream = stdout;
	    line_truncate = 256;

		MUTEX_UNLOCK(logger_mutex);

	}

	static THREAD_CALLBACK(watchdog_loop, param) {

		State* state  = (State*) param;

	    bool run = true;

	    while (run) {

	        sleep(1);

	        MUTEX_LOCK(state->watchdog_mutex);

	        run = state->watchdog_active;

	        if (state->process && state->watchdog_timeout > 0) {

		        if (!state->process->is_alive()) {
		            
		            DEBUGMSG(state, "Remote process has terminated ...\n");
		            state->watchdog_timeout = 0;

		        } else {

					state->watchdog_timeout--;

		            if (state->watchdog_timeout == 0) {
	                    DEBUGMSG(state, "Timeout reached. Stopping tracker process ...\n");
	                    state->process->stop();
		            }

		        }

			}

	        MUTEX_UNLOCK(state->watchdog_mutex);

	    }

	    return 0;

	}

	static THREAD_CALLBACK(logger_loop, param) {

		State* state  = (State*) param;

	    bool run = true;
	    int err = -1;

	    while (run) {

	        if (err == -1) {

	            run = state->logger_active;

	            MUTEX_LOCK(state->logger_mutex);

	            if (state->process && state->process->is_alive()) {

	                err = state->process->get_error();

	            }

	            MUTEX_UNLOCK(state->logger_mutex);

	            if (err == -1) {
	                sleep(0);
	                continue;
	            }

	        }

	        char chr;
	        bool flush = false;
	        if (read_stream(err, &chr, 1) != 1) {

	            err = -1;
	            flush = true;

	        } else {

	            state->stderr_length++;

	            if (state->stderr_length >= state->line_truncate && state->line_truncate > 0) {
	                
	                if (state->stderr_length == state->line_truncate)
	                    state->stderr_buffer[state->stderr_position++] = '\n';

	            } else state->stderr_buffer[state->stderr_position++] = chr;

	            flush = state->stderr_position == (LOGGER_BUFFER_SIZE-1) || chr == '\n';

	            if (chr == '\n') state->stderr_length = 0;

	        }

	        if (flush) {

	            state->stderr_buffer[state->stderr_position] = 0;

	            MUTEX_LOCK(state->logger_mutex);

	            if (state->verbosity != VERBOSITY_SILENT) {
	                fputs(state->stderr_buffer, state->logger_stream);
	                fflush(state->logger_stream);
	            }

	            state->stderr_position = 0;

	            MUTEX_UNLOCK(state->logger_mutex);

	        }

	    }

	    return 0;

	}

	static void client_logger(const char *string, int length, void* obj) {

		State* state  = (State*) obj; 

	    if (!string) {

	        state->stdout_buffer[state->stdout_position] = 0;

	        MUTEX_LOCK(state->logger_mutex);

	        fputs(state->stdout_buffer, state->logger_stream);
	        state->stdout_position = 0;
	        fflush(state->logger_stream);

	        MUTEX_UNLOCK(state->logger_mutex);

	    } else {

	        for (int i = 0; i < length; i++) {
	  
	            state->stdout_length++;

	            if (state->stdout_length >= state->line_truncate && state->line_truncate > 0) {
	                
	                if (state->stdout_length == state->line_truncate)
	                    state->stdout_buffer[state->stdout_position++] = '\n';

	            } else state->stdout_buffer[state->stdout_position++] = string[i];

	            if (state->stdout_position == (LOGGER_BUFFER_SIZE-1) || string[i] == '\n') {

	                state->stdout_buffer[state->stdout_position] = 0;

	                MUTEX_LOCK(state->logger_mutex);

	                fputs(state->stdout_buffer, state->logger_stream);
	                state->stdout_position = 0;
	                fflush(state->logger_stream);

	                MUTEX_UNLOCK(state->logger_mutex);

	                if (string[i] == '\n') state->stdout_length = 0;

	            }

	        }

	    }

	}

	map<string, string> environment;
	string command;

	bool tracking;
	ConnectionMode connection;
	VerbosityMode verbosity;

    int socket_id;
    int socket_port;

    Client* client;
	Process* process;

	int timeout;

    bool watchdog_active;
	int watchdog_timeout;

    bool logger_active;
    char stdout_buffer[LOGGER_BUFFER_SIZE];
    int stdout_position;
    char stderr_buffer[LOGGER_BUFFER_SIZE];
    int stderr_position;
    int line_truncate;
    int stderr_length;
    int stdout_length;
    FILE* logger_stream;

	THREAD logger_thread;
	THREAD_MUTEX logger_mutex;

	THREAD watchdog_thread;
	THREAD_MUTEX watchdog_mutex;

};

TrackerProcess::TrackerProcess(const string& command, map<string, string> environment, int timeout, ConnectionMode connection, VerbosityMode verbosity) {

	state = new State(command, environment, connection, verbosity, timeout);

}

TrackerProcess::~TrackerProcess() {

	delete state;

}

bool TrackerProcess::initialize(Image& image, Region& region, Properties& properties) {

	if (!ready()) throw std::runtime_error("Tracker process not alive");

    state->tracking = false;

    state->start_watchdog();

    // Initialize the tracker ...
    int result = state->client->initialize(image, region, properties);

	state->stop_watchdog();

    state->tracking = result == TRAX_OK;

    if (result == TRAX_ERROR) {
    	state->stop_process();
    }

    return result == TRAX_OK;

}


bool TrackerProcess::wait(Region& region, Properties& properties) {

	if (!ready()) throw std::runtime_error("Tracker process not alive");

	if (!state->tracking) throw std::runtime_error("Tracker not initialized yet.");

    state->start_watchdog();

	int result = state->client->wait(region, properties);

	state->stop_watchdog();

    state->tracking = result == TRAX_STATE;

    if (result == TRAX_ERROR) {
    	state->stop_process();
    }

    return result == TRAX_STATE;

}

bool TrackerProcess::frame(Image& image, Properties& properties) {

	if (!ready()) throw std::runtime_error("Tracker process not alive");

	if (!state->tracking) throw std::runtime_error("Tracker not initialized yet.");

    state->start_watchdog();

	int result = state->client->frame(image, properties);

	state->stop_watchdog();

    state->tracking = result == TRAX_OK;

    if (result == TRAX_ERROR) {
    	state->stop_process();
    }

    return result == TRAX_OK;

}

bool TrackerProcess::ready() {

	if (!state->process || !state->client) return false;

	return true;

}

bool TrackerProcess::tracking() {

	if (!ready()) return false;
	return state->tracking;

}

bool TrackerProcess::reset() {

	state->stop_process();
	return state->start_process();

}

int TrackerProcess::image_formats() {

	if (!ready()) return 0;
	return state->client->configuration().format_image;

}

int TrackerProcess::region_formats() {

	if (!ready()) return 0;
	return state->client->configuration().format_region;

}

int load_trajectory(const std::string& file, std::vector<Region>& trajectory) {

    std::ifstream input;

    input.open(file.c_str(), std::ifstream::in);

    if (!input.is_open())
        return 0;

    int elements = 0;

    while (1) {

        Region region;
        input >> region;

        if (!input.good()) break;

        trajectory.push_back(region);

        elements++;

    }

    input.close();

    return elements;

}

void save_trajectory(const std::string& file, std::vector<Region>& trajectory) {

    std::ofstream output;

    output.open(file.c_str(), std::ofstream::out);

    for (std::vector<Region>::iterator it = trajectory.begin(); it != trajectory.end(); it++) {

        output << *it;

    }

    output.close();

}


}