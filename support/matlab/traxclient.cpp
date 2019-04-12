
#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <streambuf>
#include <fstream>
#include <ostream>
#include <cmath>
#include <stdexcept>
#include <sstream>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <io.h>
#include <windows.h>

#define THREAD_MUTEX HANDLE

#define MUTEX_LOCK(M) WaitForSingleObject(M, INFINITE)
#define MUTEX_UNLOCK(M) ReleaseMutex(M)
#define MUTEX_INIT(M) (M = CreateMutex(NULL, FALSE, NULL))
#define MUTEX_DESTROY(M) WaitForSingleObject(M, INFINITE); CloseHandle(M)

#else
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define THREAD_MUTEX pthread_mutex_t
#define MUTEX_LOCK(M) pthread_mutex_lock(&M)
#define MUTEX_UNLOCK(M) pthread_mutex_unlock(&M)
#define MUTEX_INIT(M) pthread_mutex_init(&M, NULL)
#define MUTEX_DESTROY(M) pthread_mutex_destroy(&M)

#endif

#include <fcntl.h>
#include <ctype.h>

#include "helpers.h"

#include <trax/client.hpp>


#ifndef OCTAVE
#ifdef __cplusplus
extern "C" bool utIsInterruptPending();
#else
extern bool utIsInterruptPending();
#endif

#define IS_INTERRUPTED (utIsInterruptPending())

#else

#include <octave/version.h>
	// Probably required for Octave 4.1
	#define OCTAVE_USE_DEPRECATED_FUNCTIONS 1

	#if (OCTAVE_MAJOR_VERSION >= 4) && (OCTAVE_MINOR_VERSION >= 1)
		#define SET_INTERRUPT_HANDLER(H) { octave::interrupt_handler h; h.int_handler = H; h.brk_handler = H; octave::set_interrupt_handler(h); }
	#else
		#ifdef OCTINTERP_API
			#undef OCTINTERP_API
		#endif

		#include <octave/config.h>
		#define SET_INTERRUPT_HANDLER(H) { octave_interrupt_handler h; h.int_handler = H; octave_set_interrupt_handler(h); }
	#endif

	#include <octave/quit.h>
	#include <octave/sighandlers.h>

	bool octave_interrupted = false;

	void interrupt_handler(int s) {
		octave_interrupted = true;
	}

	#define IS_INTERRUPTED (octave_interrupted) // (octave_interrupt_state != 0)

#endif

using namespace std;
using namespace trax;

int fd_is_valid(int fd) {
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
	return _tell(fd) != -1 || errno != EBADF;
#else
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
#endif
}

typedef struct status {
	Region region;
	double time;
	Properties properties;
	Metadata metadata;
} status;

typedef struct command {
	ImageList image;
	Region region;
	Properties parameters;
	mxArray* data;
} command;

void on_exit() {

}

static bool exit_cache;

bool must_exit() {

	if (exit_cache)
		return true;

#ifdef OCTAVE

	exit_cache = IS_INTERRUPTED;

#else

#if MATLABVER < 904 // Disable real-time interruption check on matlab 2018a and above because it results in immediate crash

	exit_cache = IS_INTERRUPTED;

#endif

#endif

	if (exit_cache)
		mexPrintf("User termination request detected. Stopping.\n");

	return exit_cache;
}

class mexstream : public std::streambuf
{
public:
	mexstream() {
		MUTEX_INIT(mutex);
	}

	virtual ~mexstream() {
		MUTEX_DESTROY(mutex);
	};

	void push() {

		MUTEX_LOCK(mutex);

		std::string r = buffer.str();

		mexPrintf("%s", r.c_str());

		buffer.str("");

		MUTEX_UNLOCK(mutex);

	}

protected:

	stringstream buffer;

	THREAD_MUTEX mutex;

	virtual int_type overflow(int_type c) {
		MUTEX_LOCK(mutex);

		buffer.put((char)c);

		MUTEX_UNLOCK(mutex);

		return c;
	}

	virtual std::streamsize xsputn(const char* s, std::streamsize num) {

		MUTEX_LOCK(mutex);

		buffer.write(s, num);

		MUTEX_UNLOCK(mutex);

		return num;
	}

};

class nullbuffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};

class nullostream : public std::ostream
{
protected:
	nullbuffer buf;

public:
	nullostream() : std::ostream(&buf) {}
};

ConnectionMode get_connection_mode(string str) {

	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	if (str == "standard") {
		return CONNECTION_DEFAULT;
	}

	if (str == "explicit") {
		return CONNECTION_EXPLICIT;
	}

	if (str == "socket") {
		return CONNECTION_SOCKETS;
	}

	MEX_ERROR("Illegal connection type");

	return CONNECTION_DEFAULT; // Avoiding clang warnings
}

void struct_to_env(const mxArray * input, map<string, string>& env) {

	if (!mxIsStruct(input)) {
		MEX_ERROR("Parameter has to be a structure.");
	}

	for (int i = 0; i < mxGetNumberOfFields(input); i++) {

		const char* name = mxGetFieldNameByNumber(input, i);
		mxArray * value = mxGetFieldByNumber(input, 0, i);

		if (mxIsChar(value)) {
			string s = get_string(value);
			env[name] = s;
		} else MEX_ERROR("Environment values can only be strings.");
	}

}


command call_callback(const mxArray *callback, status& s, mxArray* data) {

	mxArray *lhs[4], *rhs[3], *tracker_meta, *formats, *st;

	{

		const char* fieldnames[] = {"name", "description", "family"};
		tracker_meta = mxCreateStructMatrix(1, 1, 3, (const char **) fieldnames);

		mxSetFieldByNumber(tracker_meta, 0, 0, set_string(s.metadata.tracker_name().c_str()));
		mxSetFieldByNumber(tracker_meta, 0, 1, set_string(s.metadata.tracker_description().c_str()));
		mxSetFieldByNumber(tracker_meta, 0, 2, set_string(s.metadata.tracker_family().c_str()));

	}

	{

		const char* fieldnames[] = {"region", "image"};
		formats = mxCreateStructMatrix(1, 1, 2, (const char **) fieldnames);

		mxSetFieldByNumber(formats, 0, 0, decode_region(s.metadata.region_formats()));
		mxSetFieldByNumber(formats, 0, 1, decode_image(s.metadata.image_formats()));

	}

	{
		const char* fieldnames[] = {"region", "time", "properties", "tracker", "formats", "channels"};
		st = mxCreateStructMatrix(1, 1, 6, (const char **) fieldnames);

		mxSetFieldByNumber(st, 0, 0, region_to_array(s.region));
		mxSetFieldByNumber(st, 0, 1, mxCreateDoubleScalar(s.time));
		mxSetFieldByNumber(st, 0, 2, parameters_to_struct(s.properties));
		mxSetFieldByNumber(st, 0, 3, tracker_meta);
		mxSetFieldByNumber(st, 0, 4, formats);
		mxSetFieldByNumber(st, 0, 5, decode_channels(s.metadata.channels()));
	}

	rhs[0] = const_cast<mxArray *>(callback);
	rhs[1] = const_cast<mxArray *>(st);
	rhs[2] = const_cast<mxArray *>(data);

	mexCallMATLAB(4, lhs, 3, rhs, "feval");

    mxDestroyArray(st);
    mxDestroyArray(data);

	command cmd;
	if (!mxIsEmpty(lhs[0])) {
		cmd.image = array_to_images(lhs[0], s.metadata.channels());
        mxDestroyArray(lhs[0]);
    }
	if (!mxIsEmpty(lhs[1])) {
		cmd.region = array_to_region(lhs[1]);
        mxDestroyArray(lhs[1]);
    }
	if (!mxIsEmpty(lhs[2])) {
		if (mxIsStruct(lhs[2])) {
			cmd.parameters = struct_to_parameters(lhs[2]);
		}
		if (mxIsCell(lhs[2])) {
			cmd.parameters = cell_to_parameters(lhs[2]);
		}
        mxDestroyArray(lhs[2]);
	}

	cmd.data = lhs[3];

	return cmd;
}



// Usage: result = traxclient("...", @handle, args)

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

	bool interrupted = false;
	bool debug = false;
	int timeout = 30;
	ConnectionMode connection = CONNECTION_DEFAULT;
	VerbosityMode verbosity = VERBOSITY_SILENT;
	map<string, string> environment;
	string directory, logfile;

	exit_cache = false;

	if (nlhs > 1) { MEX_ERROR("At most one output argument supported."); return; }

	if (nrhs < 2) { MEX_ERROR("At least two input arguments required."); return; }

	if (!mxIsChar (prhs[0]) || mxGetNumberOfDimensions (prhs[0]) > 2) { MEX_ERROR("First argument must be a string"); return; }

	if (!mxIsClass(prhs[1], "function_handle")) {
		mexErrMsgTxt("Second input argument must be a function handle.");
	}

	const mxArray* callback = prhs[1];
	mxArray* data = NULL;

	if ( nrhs > 1 ) {

		for (int i = 2; i < std::floor((float)nrhs / 2) * 2; i += 2) {
			switch (get_argument_code(get_string(prhs[i]))) {
			case ARGUMENT_DEBUG: verbosity = (mxGetScalar(prhs[i + 1]) != 0) ? VERBOSITY_DEBUG : VERBOSITY_DEFAULT; break;
			case ARGUMENT_TIMEOUT: timeout = (int) mxGetScalar(prhs[i + 1]); break;
			case ARGUMENT_CONNECTION: connection = get_connection_mode(get_string(prhs[i + 1])); break;
			case ARGUMENT_ENVIRONMENT: struct_to_env(prhs[i + 1], environment); break;
			case ARGUMENT_STATEOBJECT: data = mxDuplicateArray(prhs[i + 1]); break;
			case ARGUMENT_DIRECTORY: directory = get_string(prhs[i + 1]); break;
			case ARGUMENT_LOG: logfile = get_string(prhs[i + 1]); break;
			default:
				MEX_ERROR("Illegal argument.");
				return;
			}
		}

	}

#ifdef OCTAVE
	octave_interrupted = false;
	SET_INTERRUPT_HANDLER(&interrupt_handler);
#endif

	if (!data) data = mxCreateDoubleMatrix( 0, 0, mxREAL );

	mexAtExit(on_exit);

	string tracker_command = get_string(prhs[0]);

	mexstream* mexbuffer = NULL;
	std::ostream *log = NULL;

	if (!logfile.empty()) {
		if (logfile.compare("#") == 0) {
			mexbuffer = new mexstream();
			log = new ostream(mexbuffer);
		}
		else
			log = new std::ofstream(logfile.c_str(), std::ofstream::out);
	} else {
			log = new nullostream();
	}

#define LOGPUSH { if (mexbuffer) { (mexbuffer)->push();  } }

	try {

		TrackerProcess tracker(tracker_command, environment, timeout, connection, verbosity, directory, log);
		tracker.register_watchdog(must_exit);

		tracker.query();

		status st;

		st.metadata = tracker.metadata();

		st.time = 0;

		bool initialized = false;

		while (true) {

			if (IS_INTERRUPTED) {
				interrupted = true;
				throw std::runtime_error("Tracking interrupted by user");
			}

			LOGPUSH;

			if (!tracker.ready()) {
				throw std::runtime_error("Tracker process not alive anymore");
			}

			// handle callback
			command cmd = call_callback(callback, st, data);

			data = cmd.data;

			if (cmd.image.size() == 0)
				break;

			if (cmd.region) {

				if (initialized) {
					tracker.reset();
					initialized = false;
				}

				if (!tracker.initialize(cmd.image, cmd.region, cmd.parameters)) {
					throw std::runtime_error("Unable to initialize tracker.");
				}

				initialized = true;

			} else {

				if (!initialized) break;

				if (!tracker.frame(cmd.image, cmd.parameters))
					throw std::runtime_error("Unable to send new frame.");

			}

			// Start timing a frame
			timer_state begin = timer_clock();

			if (!tracker.wait(st.region, st.properties))
				throw std::runtime_error("Did not receive response.");

			st.time = timer_elapsed(begin);

			LOGPUSH;

		}

		LOGPUSH;

	} catch (const std::runtime_error &e) {

		LOGPUSH;

		if (log) {
			delete log;
			log = NULL;
		}

		if (mexbuffer) {
			delete mexbuffer;
			mexbuffer = NULL;
		}

#ifdef OCTAVE
		if (interrupted) octave_handle_signal();
#endif
		MEX_ERROR(e.what());

	}

	if (log) {
		delete log;
		log = NULL;
	}

	if (mexbuffer) {
		delete mexbuffer;
		mexbuffer = NULL;
	}

	if (nlhs > 0) plhs[0] = data;

}

