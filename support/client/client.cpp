
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <stdarg.h>

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
	WSAStartup(version, &data);
	initialized = 1;
	return;
}

__inline void sleepf(float seconds) {
	Sleep((long) (seconds * 1000.0));
}

__inline void sleep(long time) {
	Sleep(time * 1000);
}

#else

//#ifdef __APPLE__
//    #include <tcpd.h>
//#else
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#define closesocket close
//#endif

void sleepf(float seconds) {
	usleep((long) (seconds * 1000000.0));
}

#include <signal.h>

#define strcmpi strcasecmp

static void initialize_sockets(void) {}

#endif

#include "process.h"
#include "threads.h"

#define LOGGER_BUFFER_SIZE 1024

int create_server_socket(int port) {

	int sid;
	struct sockaddr_in sin;

	const char* hostname = TRAX_LOCALHOST;

	initialize_sockets();

	if ((sid = (int) socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	{
		const int enable = 1;
		if (setsockopt(sid, SOL_SOCKET, SO_REUSEADDR, (const char*) &enable, sizeof(int)) < 0) {
			perror("setsockopt");
			return -1;
		}
	}

#ifdef SO_NOSIGPIPE
	{
		const int enable = 1;
		if (setsockopt(sid, SOL_SOCKET, SO_NOSIGPIPE, (const char*) &enable, sizeof(int)) < 0) {
			perror("setsockopt");
			return -1;
		}
	}
#endif

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(hostname);

	sin.sin_port = htons(port);

	if (::bind(sid, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("bind");
		closesocket(sid);
		return -1;
	}

	if (listen(sid, 1) == -1) {
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

	bool flush = buffer.position == (LOGGER_BUFFER_SIZE - 1) || chr == '\n';

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

	return true;
}


int read_stream(int fd, char* buffer, int len) {

	fd_set readfds, writefds, exceptfds;
	struct timeval tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if (fd < 0 || fd >= FD_SETSIZE)
		return -1;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(fd, &readfds);
	FD_SET(fd, &exceptfds);

	if (select(fd + 1, &readfds, &writefds, &exceptfds, &tv) < 0)
		return -1;

	if (FD_ISSET(fd, &readfds)) {
		return read(fd, buffer, len);
	} else if (FD_ISSET(fd, &exceptfds)) {
		return -1;
	} else return 0;

}

namespace trax {

int next_available_socket_port = TRAX_DEFAULT_PORT;
class TrackerProcess::State : public Synchronized {
public:
	State(const string& command, const map<std::string, std::string> environment, ConnectionMode connection, VerbosityMode verbosity, int timeout, string directory, ostream *log):
		client(NULL), process(NULL), command(command), environment(environment), socket_id(-1), socket_port(-1),
		connection(connection), verbosity(verbosity), timeout(timeout), directory(directory) {

		timer_init();

		if (log)
			logger_stream = log;
		else
			logger_stream = &cout;

		if (connection == CONNECTION_SOCKETS) {
			// Try to create a listening socket by looking for a free port number.
			int attempts = 10;
			while (attempts--) {
				socket_id = create_server_socket(next_available_socket_port++);
				if (next_available_socket_port > 65535)
					next_available_socket_port = TRAX_DEFAULT_PORT;

				if (socket_id > 0) break;
			}
			if (socket_id < 0)
				throw std::runtime_error("Unable to configure TCP server socket.");

			socket_port = next_available_socket_port-1;

			print_debug("Socket opened successfully on port %d.", socket_port);

		}

		watchdog_active = true;
		logger_active = true;

		stop_watchdog();
		reset_logger();

		CREATE_THREAD(watchdog_thread, watchdog_loop, this);
		CREATE_THREAD(logger_thread, logger_loop, this);

	}

	~State() {

		print_debug("Cleaning up.");

		watchdog_active = false;
		logger_active = false;

		RELEASE_THREAD(watchdog_thread);
		RELEASE_THREAD(logger_thread);

		stop_process();

		if (client) {
			delete client;
			client = NULL;
		}

		if (connection == CONNECTION_SOCKETS) {
			print_debug("Closing server socket.");
			destroy_server_socket(socket_id);
		}

	}

	bool process_running() {
		Lock lock(process_state_mutex);
		return process && process->is_alive();
	}

	bool start_process() {

		if (process_running()) return false;

		if (client) {
			delete client;
			client = NULL;
			print_debug("Deleted stale client");
		}

		print_debug("Creating process %s", command.c_str());

		process_state_mutex.acquire();
		process = new Process(command, connection == CONNECTION_EXPLICIT);
		if (!directory.empty()) {
			print_debug("Working directory is %s", directory.c_str());
			process->set_directory(directory);
		}
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
		print_debug("Starting process");
		started = process->start();

		if (!started) {
			print_debug("Unable to start process");
			throw std::runtime_error("Unable to start the tracker process");
		} else {

			sleepf(0.1);
			if (!process->is_alive()) {
				print_debug("Unable to start process");
				throw std::runtime_error("Unable to start the tracker process");
			}

			start_watchdog();

			Logging logger = trax_no_log;

			if (verbosity != VERBOSITY_SILENT)
				logger = Logging(client_logger, this, 0);

			if (connection == CONNECTION_SOCKETS) {
				print_debug("Setting up TraX with TCP socket connection");
				client = new Client(socket_id, logger, timeout);
			} else {
				print_debug("Setting up TraX with %s streams connection", (connection == CONNECTION_EXPLICIT) ? "dedicated" : "standard");
				client = new Client(process->get_output(), process->get_input(), logger);
			}
			// TODO: check tracker exit state
			if (!(*client)) {
				print_debug("Unable to establish connection.");
				throw std::runtime_error("Unable to establish connection.");
			}

			stop_watchdog();

			print_debug("Tracker process ID: %d", process->get_handle());
			int server_version = 0;

			client->get_parameter(TRAX_PARAMETER_VERSION, &server_version);

			if (server_version > TRAX_VERSION) throw std::runtime_error("Unsupported protocol version");

			print_debug("Connection with tracker established.");

			return true;

		}

	}


	bool stop_process() {

		int exit_status = -1;

		stop_watchdog();

		Lock lock(process_state_mutex);

		if (client != NULL) {
			print_debug("Trying to stop process using protocol.");
			client->terminate();
		}

		if (process) {

			for (int i = 0; i < 5; i++) {
				if (!process->is_alive(&exit_status)) break;
				sleepf(0.1);
			}

			if (process->is_alive(&exit_status)) {
				print_debug("Trying to terminate process nicely.");
				process->stop();
				for (int i = 0; i < 5; i++) {
					if (!process->is_alive(&exit_status)) break;
					sleepf(0.1);
				}

				if (process->is_alive(&exit_status)) {
					print_debug("Escalating to brute-force termination.");
					process->kill();
					sleepf(0.1);
					process->is_alive(&exit_status);
				}
			}

			flush_streams();

			delete process;
			process = NULL;

			print_debug("Process should be terminated.");

			reset_logger();

			print_debug("Stopping logger.");

			if (exit_status == 0) {
				print_debug("Tracker exited normally.");
				return true;
			} else if (exit_status < 0) {
				print_debug("Tracker exited (stopped by signal %d)", -exit_status);
				return false;
			} else {
				print_debug("Tracker exited (exit code %d)", exit_status);
				return false;
			}
		}

		return true;

	}

	void reset_watchdog(int timeout) {

		watchdog_timeout = timeout * 10;

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

		print_debug("Flushing streams");

		char buffer[LOGGER_BUFFER_SIZE];

		for (int i = 0; i < 10; i++) {

			int read = read_stream(process->get_error(), buffer, LOGGER_BUFFER_SIZE);

			if (read <= 0) break;

			client_logger(buffer, read, this);

			sleepf(0.05);

		}

		flush_condition.wait(1000);

	}

	void print_debug(const char *format, ...) {
		if (verbosity != VERBOSITY_DEBUG || !logger_stream)
			return;

		va_list args;
		va_start(args, format);

		int size = 1024;
		char* buffer = NULL;

#ifdef _MSC_VER

		size = _vscprintf(format, args) + 1;
		buffer = (char*) malloc(sizeof(char) * size);
		_vsnprintf_s(buffer, size, _TRUNCATE, format, args);

#else

		buffer = (char*) malloc(sizeof(char) * size);

		int required = vsnprintf(buffer, size, format, args) + 1;
		if (required >= size) {
			size = required;
			buffer = (char*) realloc(buffer, sizeof(char) * size);
			required = vsnprintf(buffer, size, format, args);
		}
#endif
		buffer[size - 1] = 0;

		MUTEX_SYNCHRONIZE(logger_mutex) {
			*logger_stream << "CLIENT: " << buffer << std::endl;
		}

		free(buffer);

		va_end(args);
	}

	static THREAD_CALLBACK(watchdog_loop, param) {

		State* state  = (State*) param;

		bool run = true;

		while (run) {

			sleepf(0.1);

			state->watchdog_mutex.acquire();

			run = state->watchdog_active;

			bool terminate = !state->process_running();;

			for (vector<WatchdogCallback*>::iterator c = state->callbacks.begin(); c != state->callbacks.end(); c++) {
				terminate |= (*c)();
			}

			if (terminate) {
				//state->print_debug("Termination requested externally ...");
				state->stop_process();
			} else {

				if (state->watchdog_timeout > 0) {

					state->watchdog_timeout--;

					if (state->watchdog_timeout == 0) {
						//state->print_debug("Timeout reached. Stopping tracker process ...");
						state->stop_process();
					}

				}
			}

			state->watchdog_mutex.release();

		}

		//state->print_debug("Stopping watchdog thread");

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

				MUTEX_SYNCHRONIZE(state->logger_mutex) {

					line_buffer_flush(state->stderr_buffer, (state->verbosity != VERBOSITY_SILENT) ? state->logger_stream : NULL);

				}

				if (err == -1) state->flush_condition.notify();

			}

		}

		state->print_debug("Stopping logger thread");

		return 0;

	}

	static void client_logger(const char *string, int length, void* obj) {

		State* state  = (State*) obj;

		if (!string) {

			MUTEX_SYNCHRONIZE(state->logger_mutex) {

				line_buffer_flush(state->stdout_buffer, state->logger_stream);

			}

		} else {

			for (int i = 0; i < length; i++) {

				if (line_buffer_push(state->stdout_buffer, string[i], state->line_truncate)) {

					MUTEX_SYNCHRONIZE(state->logger_mutex) {

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

	Client* client;
	Process* process;

	vector<WatchdogCallback*> callbacks;
	string command;
	map<string, string> environment;

	int socket_id;
	int socket_port;

	bool tracking;
	ConnectionMode connection;
	VerbosityMode verbosity;

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

	string directory;

};

TrackerProcess::TrackerProcess(const string& command, const map<string, string> environment, int timeout,
                               ConnectionMode connection, VerbosityMode verbosity, string directory, ostream *log) {

	state = new State(command, environment, connection, verbosity, timeout, directory, log);

}

TrackerProcess::~TrackerProcess() {

	if (state) delete state;
	state = NULL;

}

bool TrackerProcess::query() {

	return state->start_process();

}

bool TrackerProcess::initialize(const ImageList& image, const Region& region, const Properties& properties) {

	query();

	if (!ready()) throw std::runtime_error("Tracker process not alive");

	state->tracking = false;

	state->start_watchdog();

	// Initialize the tracker ...
	int result = state->client->initialize(image, region, properties);

	state->stop_watchdog();

	state->tracking = result == TRAX_OK;

	if (result == TRAX_ERROR) {
		state->stop_process();
		std::runtime_error("Unable to initialize tracker: " + state->client->get_error());
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
		std::runtime_error("Unable to retrieve response: " + state->client->get_error());
	}

	return result == TRAX_STATE;

}

bool TrackerProcess::frame(const ImageList& image, const Properties& properties) {

	if (!ready()) throw std::runtime_error("Tracker process not alive");

	if (!state->tracking) throw std::runtime_error("Tracker not initialized yet.");

	state->start_watchdog();

	int result = state->client->frame(image, properties);

	state->stop_watchdog();

	state->tracking = result == TRAX_OK;

	if (result == TRAX_ERROR) {
		state->stop_process();
		std::runtime_error("Unable to send new frame: " + state->client->get_error());
	}

	return result == TRAX_OK;

}

bool TrackerProcess::ready() {

	if (!state || !state->process_running() || !state->client->is_alive()) return false;

	return true;

}

bool TrackerProcess::tracking() {

	if (!ready()) return false;
	return state->tracking;

}

bool TrackerProcess::reset() {

	state->print_debug("Resetting program.");

	state->stop_process();
	return state->start_process();

}

Metadata TrackerProcess::metadata() {

	if (!ready()) throw std::runtime_error("Tracker not initialized yet.");
	return state->client->metadata();

}

void TrackerProcess::register_watchdog(WatchdogCallback* callback) {

	state->register_callback(callback);

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
