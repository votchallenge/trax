
#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
#endif
#include <fcntl.h>
#include <ctype.h>

#include "helpers.h"

using namespace std;
using namespace trax;

Server* handle = NULL;

int fd_is_valid(int fd) {
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
    return _tell(fd) != -1 || errno != EBADF;
#else
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
#endif
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    if ( nrhs < 1 ) { MEX_ERROR("At least one input argument required."); return; }

    if (! mxIsChar (prhs[0]) || mxGetNumberOfDimensions (prhs[0]) > 2) { MEX_ERROR("First argument must be a string"); return; }

    string operation = get_string(prhs[0]);

	std::transform(operation.begin(), operation.end(), operation.begin(), ::tolower);

    if (operation == "setup") {

        if ( nrhs < 3 ) { MEX_ERROR("At least three parameters required."); return; }

        if ( nlhs > 1 ) { MEX_ERROR("At most one output argument supported."); return; }

        std::string tracker_name, tracker_description, tracker_family;

        int channels = TRAX_CHANNEL_COLOR;

        std::map<std::string, std::string> custom;

        if ( nrhs > 3 ) {

            for (int i = 3; i < std::floor((float)nrhs/2) * 2; i+=2) {
                string key = get_string(prhs[i]);
                switch (get_argument_code(key)) {
                case ARGUMENT_TRACKERNAME: tracker_name = get_string(prhs[i+1]); break;
                case ARGUMENT_TRACKERDESCRIPTION: tracker_description = get_string(prhs[i+1]); break;
                case ARGUMENT_TRACKERFAMILY: tracker_family = get_string(prhs[i+1]); break;
                case ARGUMENT_CHANNELS: channels = get_flags(prhs[i+1], get_channel_code); break;
                case ARGUMENT_CUSTOM:
                    if (key.rfind("trax.", 0) == 0) {
                        MEX_ERROR("Illegal argument.");
                        return;
                    } else {
                        custom[key] = get_string(prhs[i+1]);
                    }
                    break;
				default:
					MEX_ERROR("Illegal argument.");
					return;
				}
            }

        }

        int region_formats = get_flags(prhs[1], get_region_code);
        int image_formats = get_flags(prhs[2], get_image_code);

        Metadata metadata(region_formats, image_formats, channels,
            tracker_name, tracker_description, tracker_family);

        typedef std::map<std::string, std::string>::const_iterator it_type;
        for(it_type iterator = custom.begin(); iterator != custom.end(); iterator++) {
            metadata.set_custom(iterator->first, iterator->second);
        }

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

        if (!getenv("TRAX_SOCKET")) {
            MEX_ERROR("Socket information not available. Enable socket communication in TraX client.");
            return;
        }

#endif

        handle = new Server(metadata, trax_no_log);

        if (!handle)
            MEX_ERROR("Unable to initialize protocol.");

        if (!handle->is_alive()) {
            std::string tmp("Unable to initialize protocol: " + handle->get_error());
            MEX_ERROR(tmp.c_str());
        }

        if (nlhs == 1)
            plhs[0] = mxCreateLogicalScalar((bool) handle);

    } else if (operation == "wait") {

		bool propstruct = false;

        if (!handle) { MEX_ERROR("Protocol not initialized."); return; }

        if ( nrhs > 1 ) {

			for (int i = 1; i < std::floor((float)nrhs/2) * 2; i+=2) {
				switch (get_argument_code(get_string(prhs[i]))) {
				case ARGUMENT_PROPSTRUCT: propstruct = mxGetScalar(prhs[i+1]) > 0; break;
				default:
					MEX_ERROR("Illegal argument.");
					return;
				}
			}

		}

        if ( nlhs > 3 ) { MEX_ERROR("At most three output argument supported."); return; }

        ImageList img;
        Region reg;
        Properties prop;

        int tr = handle->wait(img, reg, prop);

        if (tr == TRAX_INITIALIZE) {
            if (nlhs > 0) plhs[0] = images_to_array(img);
            if (nlhs > 1) plhs[1] = region_to_array(reg);
            if (nlhs > 2) plhs[2] = (propstruct) ? parameters_to_struct(prop) : parameters_to_cell(prop);

        } else if (tr == TRAX_FRAME) {

            if (nlhs > 0) plhs[0] = images_to_array(img);
            if (nlhs > 1) plhs[1] = MEX_CREATE_EMTPY;
            if (nlhs > 2) plhs[2] = (propstruct) ? parameters_to_struct(prop) : parameters_to_cell(prop);

        } else if (tr == TRAX_ERROR) {

            std::string tmp("Protocol error while waiting: " + handle->get_error());
            MEX_ERROR(tmp.c_str());

        } else {

            if (nlhs > 0) plhs[0] = MEX_CREATE_EMTPY;
            if (nlhs > 1) plhs[1] = MEX_CREATE_EMTPY;
            if (nlhs > 2) plhs[2] = MEX_CREATE_EMTPY;

            if (handle) delete handle;
			handle = NULL;

        }

    } else if (operation == "status") {

        if (!handle) { MEX_ERROR("Protocol not initialized."); return; }

        if ( nrhs > 3 ) { MEX_ERROR("Too many parameters."); return; }

        if ( nrhs < 2 ) { MEX_ERROR("Must specify region parameter."); return; }

        if ( nlhs > 1 ) { MEX_ERROR("One output parameter allowed."); return; }

        Region reg = array_to_region(prhs[1]);

        if (!reg) { MEX_ERROR("Illegal region format."); return; }

        Properties prop = ( nrhs == 3 ) ? (mxIsStruct(prhs[2]) ? struct_to_parameters(prhs[2]) : cell_to_parameters(prhs[2])) : Properties();

        if (handle->reply(reg, prop) == TRAX_ERROR) {
            std::string tmp("Unable to send reply: " + handle->get_error());
            MEX_ERROR(tmp.c_str());
        }
        
        if (nlhs == 1)
            plhs[0] = mxCreateLogicalScalar(handle != NULL);

    } else if (operation == "quit") {

        if (!handle)
            return;

        std::string reason;

        if (nrhs > 2)
            MEX_ERROR("Only one optional string argument allowed.");
        else
            reason = get_string(prhs[1]);

        if (handle->terminate(reason) == TRAX_ERROR) {
            std::string tmp("Unable to terminate protocol: " + handle->get_error());
            MEX_ERROR(tmp.c_str());
        }

		if (handle) delete handle;
		handle = NULL;

    } else {
        MEX_ERROR("Unknown operation.");
    }

}

