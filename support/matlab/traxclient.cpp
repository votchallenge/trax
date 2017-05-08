
#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <streambuf>
#include <fstream>
#include <ostream>
#include <cmath>
#include <stdexcept>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
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

#define IS_INTERRUPTED utIsInterruptPending()

#else

#ifdef OCTINTERP_API
#undef OCTINTERP_API
#endif

#include <octave/config.h>
#include <octave/quit.h>

inline bool _octave_quit (void) {
	if (octave_signal_caught)
	{
		octave_signal_caught = 0;
		return true;
	}
	return false;
};

#define IS_INTERRUPTED _octave_quit()

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
	Image image;
	Region region;
	Properties parameters;
	mxArray* data;
} command;

void on_exit() {

}


bool must_exit() {

	bool exit = IS_INTERRUPTED;

	if (exit)
		mexPrintf("User termination request detected. Stopping.\n");

	return exit;
} 

class mexstream : public std::streambuf
{
public:
	mexstream() {}

protected:
	virtual int_type overflow(int_type c) {
	    if (c != EOF) {
	      mexPrintf("%.1s",&c);
	    }
	    return 1;
	}

	virtual std::streamsize xsputn(const char* s, std::streamsize num) {
		
		mexPrintf("%.*s", num, s);

		return num;
	}
};

class mexostream : public std::ostream
{
protected:
	mexstream buf;

public:
	mexostream() : std::ostream(&buf) {}
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


command call_callback(const mxArray *callback, status& s, const mxArray* data) {

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
		const char* fieldnames[] = {"region", "time", "properties", "tracker", "formats"};
		st = mxCreateStructMatrix(1, 1, 5, (const char **) fieldnames);

		mxSetFieldByNumber(st, 0, 0, region_to_array(s.region));
		mxSetFieldByNumber(st, 0, 1, mxCreateDoubleScalar(s.time));
		mxSetFieldByNumber(st, 0, 2, parameters_to_struct(s.properties));
		mxSetFieldByNumber(st, 0, 3, tracker_meta);
		mxSetFieldByNumber(st, 0, 4, formats);
	}

	rhs[0] = const_cast<mxArray *>(callback);
	rhs[1] = const_cast<mxArray *>(st);
	rhs[2] = const_cast<mxArray *>(data);

	mexCallMATLAB(4, lhs, 3, rhs, "feval");

	command cmd;
	if (!mxIsEmpty(lhs[0]))
		cmd.image = array_to_image(lhs[0]);
	if (!mxIsEmpty(lhs[1]))
		cmd.region = array_to_region(lhs[1]);
	if (!mxIsEmpty(lhs[2])) {
		if (mxIsStruct(lhs[2])) {
			cmd.parameters = struct_to_parameters(lhs[2]);
		}
		if (mxIsCell(lhs[2])) {
			cmd.parameters = cell_to_parameters(lhs[2]);
		}
	}

	//cmd.parameters.set("wait", 3);
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
			case ARGUMENT_DEBUG: verbosity = (mxGetScalar(prhs[i + 1]) != 0) ? VERBOSITY_DEBUG : VERBOSITY_SILENT; break;
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

	if (!data) data = mxCreateDoubleMatrix( 0, 0, mxREAL );

	mexAtExit(on_exit);

	string tracker_command = get_string(prhs[0]);

	std::ostream *log = NULL;

	if (!logfile.empty()) {
		log = new std::ofstream(logfile.c_str(), std::ofstream::out);
	} else {
		log = new mexostream();
	}

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

			if (!tracker.ready()) {
				throw std::runtime_error("Tracker process not alive anymore");
			}

			// handle callback
			command cmd = call_callback(callback, st, data);

			data = cmd.data;

			if (!cmd.image)
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

		}

	} catch (std::runtime_error e) {

		if (log) {
			delete log;
			log = NULL;
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

	if (nlhs > 0) plhs[0] = data;

}

