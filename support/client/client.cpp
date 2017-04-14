
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>

#include <trax/client.hpp>

class null_out_buf : public std::streambuf {
    public:
        virtual std::streamsize xsputn (const char * s, std::streamsize n) {
            return n;
        }
        virtual int overflow (int c) {
            return 1;
        }
};

class null_out_stream : public std::ostream {
    public:
        null_out_stream() : std::ostream (&buf) {}
    private:
        null_out_buf buf;
};

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>

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

#define DEBUGMSG(state, ...) if (state->verbosity == VERBOSITY_DEBUG) { printf("CLIENT: "); printf(__VA_ARGS__); }

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

typedef struct line_buffer {

    char buffer[LOGGER_BUFFER_SIZE];
    int position;
    int length;

} line_buffer;

#define RESET_LINE_BUFFER(B) {B.position = 0; B.length = 0;}

bool line_buffer_push(line_buffer& buffer, char chr, int truncate = -1) {

    buffer.length++;

    if (buffer.length >= truncate && truncate > 0) {

        if (buffer.length == truncate)
            buffer.buffer[buffer.position++] = '\n';

    } else buffer.buffer[buffer.position++] = chr;

    bool flush = buffer.position == (LOGGER_BUFFER_SIZE-1) || chr == '\n';

    if (chr == '\n') buffer.length = 0;

    return flush;

}

bool line_buffer_flush(line_buffer& buffer, ostream* out) {

    buffer.buffer[buffer.position] = 0;

    if (out != NULL) {
	    out->write(buffer.buffer, buffer.position);
    	out->flush();
	}

    buffer.position = 0;

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

class TrackerProcess::State : public Synchronized {
public:
	State(const string& command, const map<std::string, std::string> environment, ConnectionMode connection, VerbosityMode verbosity, int timeout):
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

        logger_stream = &cout;

        client = NULL;
        process = NULL;
        watchdog_active = true;
        logger_active = true;

        stop_watchdog();
        reset_logger();

        CREATE_THREAD(watchdog_thread, watchdog_loop, this);
        CREATE_THREAD(logger_thread, logger_loop, this);

        start_process();
	}

	~State() {

		watchdog_active = false;
		logger_active = false;

		stop_process();

	    RELEASE_THREAD(watchdog_thread);
	    RELEASE_THREAD(logger_thread);


	    DEBUGMSG(this, "Cleaning up.\n");

	    if (connection == CONNECTION_SOCKETS) {
	        destroy_server_socket(socket_id);
	    }

	}

	bool process_running() {
		Lock lock(process_state_mutex);
		return process && process->is_alive();
	}

	bool start_process() {

		if (process && process) return false;

		DEBUGMSG(this, "Starting process %s\n", command.c_str());

		process_state_mutex.acquire();
	    process = new Process(command, connection == CONNECTION_EXPLICIT);
	    process_state_mutex.release();

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

	    bool started = false;
	    started = process->start();

	    if (!started) {
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
	        if (!(*client)) {
				throw std::runtime_error("Unable to establish connection.");
	        } 

			stop_watchdog();

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

        sleep(0);

		if (client) {
			delete client;
			client = NULL;
		}

        sleep(0);

        Lock lock(process_state_mutex);

		if (process) {
		    int exit_status;

		    DEBUGMSG(this, "Stopping.\n");

		    process->stop(false);
			flush_streams();
			process->stop(true);

		    process->is_alive(&exit_status);

		    delete process;
		    process = NULL;

		    reset_logger();

		    if (exit_status == 0) {
		        DEBUGMSG(this, "Tracker exited normally.\n");
		        return true;
		    } else {
		        DEBUGMSG(this, "Tracker exited (exit code %d)\n", exit_status);
		        return false;
		    }
		}

		return true;

	}

	void reset_watchdog(int timeout) {

	    watchdog_timeout = timeout;

	}

	void start_watchdog() {

		reset_watchdog(timeout);

	}


	void stop_watchdog() {

		reset_watchdog(0);

	}

	void reset_logger() {

	    Lock lock(logger_mutex);

		RESET_LINE_BUFFER(stderr_buffer);
		RESET_LINE_BUFFER(stdout_buffer);

	    line_truncate = 256;

	}

	void flush_streams() {

	    char buffer[LOGGER_BUFFER_SIZE];
	    bool flush = false;
	   
        while (true) {

			int read = read_stream(process->get_error(), buffer, LOGGER_BUFFER_SIZE);

			if (read <= 0) break;

			client_logger(buffer, read, this);

        }

        flush_condition.wait(1000);

	}

	static THREAD_CALLBACK(watchdog_loop, param) {

		State* state  = (State*) param;

	    bool run = true;

	    while (run) {

	        sleep(1);

	        state->watchdog_mutex.acquire();

	        run = state->watchdog_active;

	        if (state->process) {

		        if (!state->process->is_alive()) {

		            DEBUGMSG(state, "Remote process has terminated ...\n");
					state->stop_process(); // Do not close socket so that any leftover data can be read

		        } else {


			        bool terminate = false;

			        for (vector<WatchdogCallback*>::iterator c = state->callbacks.begin(); c != state->callbacks.end(); c++) {
			        	terminate |= (*c)();
			        }

			        if (terminate) {
	                    DEBUGMSG(state, "Termination requested externally ...\n");
	                    state->stop_process();
			        }

			        if (state->watchdog_timeout > 0) {

						state->watchdog_timeout--;

			            if (state->watchdog_timeout == 0) {
		                    DEBUGMSG(state, "Timeout reached. Stopping tracker process ...\n");
		                    state->stop_process();
			            }

		            }

		        }

			}

	        state->watchdog_mutex.release();

	    }

		DEBUGMSG(state, "Stopping watchdog thread\n");

	    return 0;

	}

	static THREAD_CALLBACK(logger_loop, param) {

		State* state  = (State*) param;

	    bool run = true;
	    int err = -1;

	    while (run) {

			run = state->logger_active;

	        if (err == -1) {
 
	            if (state->process_running()) {

	                err = state->process->get_error();

	            } else {

	            	state->flush_condition.notify();

	            }

	            if (err == -1) {
	                sleep(0);
	                continue;
	            }

	        }

	        char chr;
	        bool flush = false;
	        int read = read_stream(err, &chr, 1);
	        if (read != 1) {

	            if (state->process_running()) err =  -1;
	            flush = true;

	        } else {

				flush = line_buffer_push(state->stderr_buffer, chr, state->line_truncate);

	        }

	        if (flush) {

	            SYNCHRONIZE(state->logger_mutex) {

	            	line_buffer_flush(state->stderr_buffer, state->verbosity != VERBOSITY_SILENT ? state->logger_stream : NULL);	            

	            }

				if (err == -1) state->flush_condition.notify();

	        }

	    }

	    DEBUGMSG(state, "Stopping logger thread\n");

	    return 0;

	}

	static void client_logger(const char *string, int length, void* obj) {

		State* state  = (State*) obj;

	    if (!string) {

            SYNCHRONIZE(state->logger_mutex) {

            	line_buffer_flush(state->stdout_buffer, state->logger_stream);

            }

	    } else {

	        for (int i = 0; i < length; i++) {

	        	if (line_buffer_push(state->stdout_buffer, string[i], state->line_truncate)) {

		            SYNCHRONIZE(state->logger_mutex) {

		            	line_buffer_flush(state->stdout_buffer, state->logger_stream);

		            }

	        	}

	        }

	    }

	}

	void register_callback(WatchdogCallback* callback) {
		Lock lock(watchdog_mutex);

		callbacks.push_back(callback);

	}

	void set_log(ostream *stream) {

		SYNCHRONIZE(logger_mutex) {

			logger_stream = stream;

		}

		reset_logger();

	}


	vector<WatchdogCallback*> callbacks;
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
    line_buffer stdout_buffer;
    line_buffer stderr_buffer;
    int line_truncate;

    ostream* logger_stream;

	Mutex logger_mutex;
	Mutex watchdog_mutex;

	Mutex process_state_mutex;

	THREAD logger_thread;

	THREAD watchdog_thread;
	Signal flush_condition;

};

TrackerProcess::TrackerProcess(const string& command, const map<string, string> environment, int timeout, ConnectionMode connection, VerbosityMode verbosity) {

	state = new State(command, environment, connection, verbosity, timeout);

}

TrackerProcess::~TrackerProcess() {

	delete state;

}

bool TrackerProcess::initialize(const Image& image, const Region& region, const Properties& properties) {

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

bool TrackerProcess::frame(const Image& image, const Properties& properties) {

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

void TrackerProcess::register_watchdog(WatchdogCallback* callback) {

	state->register_callback(callback);

}

void TrackerProcess::set_log(ostream *stream) {

	state->set_log(stream);

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
