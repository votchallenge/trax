
#include <stdio.h>
#include <string.h>
#include <string>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
#endif
#include <fcntl.h>

#include <trax.h>

#include "mex.h"

#define MEX_TEST_DOUBLE(I) (mxGetClassID(prhs[I]) == mxDOUBLE_CLASS)
#define MEX_TEST_VECTOR(I) (mxGetNumberOfDimensions(prhs[I]) == 2 && mxGetM(prhs[I]) == 1)
#define MEX_CREATE_EMTPY mxCreateDoubleMatrix(0, 0, mxREAL)
#define MEX_ERROR(M) mexErrMsgTxt(M)

using namespace std;

trax_handle* handle = NULL;

string get_string(const mxArray *arg) {

    if (mxGetM(arg) != 1) {
        MEX_ERROR("Must be an array of chars");
        return string();
    }

    int l = (int) mxGetN(arg);

    char* cstr = (char *) malloc(sizeof(char) * (l + 1));

    mxGetString(arg, cstr, (l + 1));

    string str(cstr);

    free(cstr);

    return str;
}

mxArray* set_string(const char* str) {

    return mxCreateString(str);

}

trax_properties* struct_to_parameters(const mxArray * input) {

    if (!mxIsStruct(input)) {
        MEX_ERROR("Parameter has to be a structure.");
        return NULL;
    }

    trax_properties* prop = trax_properties_create();

    for (int i = 0; i < mxGetNumberOfFields(input); i++) {

        const char* name = mxGetFieldNameByNumber(input, i);

        mxArray * value = mxGetFieldByNumber(input, 0, i);

        if (mxIsChar(value)) {
            string s = get_string(value);
            trax_properties_set(prop, name, s.c_str());
        } else {
            trax_properties_set_float(prop, name, (float) mxGetScalar(value));
        }
    }

    return prop;
}

trax_properties* cell_to_parameters(const mxArray * input) {

    if (!mxIsCell(input)) {
        MEX_ERROR("Parameter has to be a cell array.");
        return NULL;
    }

    int length = mxGetM(input);
    int n = mxGetN(input);

    if ( min(mxGetM(input), mxGetN(input)) != 1 )
        mexErrMsgTxt("Array must be a vector");


    trax_properties* prop = trax_properties_create();

    for (int i = 0; i < length; i++) {
        mxArray* key = mxGetCell(input, i);
        mxArray* value = mxGetCell(input, i + length);

        string skey = get_string(key);

        if (mxIsChar(value)) {
            string s = get_string(value);
            trax_properties_set(prop, skey.c_str(), s.c_str());
        } else {
            trax_properties_set_float(prop, skey.c_str(), (float) mxGetScalar(value));
        }

    }

    return prop;
}


typedef struct _param_copy {
    mxArray *array;
    int pos;
    int length;
} _param_copy;

void _param_count_enumerator(const char *key, const char *value, const void *obj) {

    int* length = (int *) obj;

    (*length) ++;

}

void _param_copy_enumerator(const char *key, const char *value, const void *obj) {

    _param_copy* tmp = (_param_copy*) obj;

    const char** cpkey = (const char **) & (key);
    mxArray* mxKey = mxCreateCharMatrixFromStrings(1, cpkey);

    const char** cpval = (const char **) & (value);
    mxArray* mxVal = mxCreateCharMatrixFromStrings(1, cpval);

    mxSetCell(tmp->array, tmp->pos, mxKey);
    mxSetCell(tmp->array, tmp->pos + tmp->length, mxVal);

    tmp->pos++;

}

mxArray* parameters_to_cell(trax_properties * input) {

    int length = 0;

    trax_properties_enumerate(input, _param_count_enumerator, &length);

    _param_copy tmp;

    tmp.array = mxCreateCellMatrix(length, 2);
    tmp.pos = 0;
    tmp.length = length;

    trax_properties_enumerate(input, _param_copy_enumerator, &tmp);

    return tmp.array;

}

trax_region* array_to_region(const mxArray* input) {

    trax_region* p = NULL;
    double *r = (double*)mxGetPr(input);
    int l = mxGetN(input);

    if (l % 2 == 0 && l > 6) {

        p = trax_region_create_polygon(l / 2);

        for (int i = 0; i < l / 2; i++) {
            trax_region_set_polygon_point(p, i, r[i * 2], r[i * 2 + 1]);
        }

    } else if (l == 4) {

        p = trax_region_create_rectangle(r[0], r[1], r[2], r[3]);

    }

    return p;

}

mxArray* region_to_array(const trax_region* region) {

    mxArray* val = NULL;

    switch (trax_region_get_type(region)) {
    case TRAX_REGION_RECTANGLE: {
        val = mxCreateDoubleMatrix(1, 4, mxREAL);
        double *p = (double*) mxGetPr(val); float x, y, width, height;
        trax_region_get_rectangle(region, &x, &y, & width, &height);
        p[0] = x; p[1] = y; p[2] = width; p[3] = height;

        break;
    }
    case TRAX_REGION_POLYGON: {
        val = mxCreateDoubleMatrix(1, trax_region_get_polygon_count(region) * 2, mxREAL);
        double *p = (double*) mxGetPr(val);

        for (int i = 0; i < trax_region_get_polygon_count(region); i++) {
            float x, y;
            trax_region_get_polygon_point(region, i, &x, &y);
            p[i * 2] = x; p[i * 2 + 1] = y;
        }

        break;
    }
    case TRAX_REGION_SPECIAL: {
        val = mxCreateDoubleMatrix(1, 1, mxREAL);
        double *p = (double*) mxGetPr(val);
        p[0] = trax_region_get_special(region);
        break;
    }
    case TRAX_REGION_MASK: {
        // TODO
        MEX_ERROR("Mask conversion not implemented yet.");
        break;
    }
    }

    return val;
}


mxArray* image_to_array(const trax_image* img) {

    mxArray* val = NULL;

    switch (trax_image_get_type(img)) {
    case TRAX_IMAGE_PATH: {
        const char* cp = trax_image_get_path(img);
        val = mxCreateCharMatrixFromStrings(1, &cp);
        break;
    }
    case TRAX_IMAGE_URL: {
        const char* cp = trax_image_get_url(img);
        val = mxCreateCharMatrixFromStrings(1, &cp);
        break;
    }
    case TRAX_IMAGE_MEMORY: {
        int width, height;
        int format, depth;
        mwSize dims[3];
        mxClassID classid;
        trax_image_get_memory_header(img, &width, &height, &format);

        dims[0] = height;
        dims[1] = width;
        switch (format) {
        case TRAX_IMAGE_MEMORY_GRAY8: {
            dims[2] = 1;
            val = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
            char *data = (char*) mxGetPr(val);
            const char* source = trax_image_get_memory_row(img, 0);
            for (int i = 0; i < width; i++) {
                for (int j = 0; j < height; j++) {
                    data[j + height * i] = source[i + j * width];
                }
            }

            break;
        }
        case TRAX_IMAGE_MEMORY_GRAY16: {
            dims[2] = 1;

            val = mxCreateNumericArray(3, dims, mxUINT16_CLASS, mxREAL);
            unsigned short *data = (unsigned short*) mxGetPr(val);
            const unsigned short* source = (const unsigned short*) trax_image_get_memory_row(img, 0);

            for (int i = 0; i < width; i++) {
                for (int j = 0; j < height; j++) {
                    data[j + height * i] = source[i + j * width];
                }
            }

            break;
        }
        case TRAX_IMAGE_MEMORY_RGB: {
            dims[2] = 3;
            val = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
            char *data = (char*) mxGetPr(val);
            const char* source = trax_image_get_memory_row(img, 0);

            for (int i = 0; i < width; i++) {
                for (int j = 0; j < height; j++) {
                    for (int k = 0; k < 3; k++) {
                        data[j + height * (i + k * width)] = source[3 * (i + j * width) + k];
                    }
                }
            }

            break;
        }
        }

        break;
    }


    case TRAX_IMAGE_BUFFER: {
        int length;
        int format;
        mwSize dims[1];
        const char* fieldnames[2];
        mxArray* formatstring;
        const char* source = trax_image_get_buffer(img, &length, &format);

        dims[0] = length;

        switch (format) {

        case TRAX_IMAGE_BUFFER_PNG: {
            formatstring = set_string("png");
            break;
        }
        case TRAX_IMAGE_BUFFER_JPEG: {
            formatstring = set_string("jpeg");
            break;
        }
        default: {
            formatstring = set_string("unknown");
            break;
        }
        }
        mxArray* dval = mxCreateNumericArray(1, dims, mxUINT8_CLASS, mxREAL);
        char *data = (char*) mxGetPr(dval);
        memcpy(data, source, length);

        fieldnames[0] = "format";
        fieldnames[1] = "data";
        val = mxCreateStructMatrix(1, 1, 2, fieldnames);

        mxSetFieldByNumber(val, 0, 0, formatstring);
        mxSetFieldByNumber(val, 0, 1, dval);
        break;
    }
    }

    return val;

}

int get_region_code(string str) {

    if (str == "rectangle") {
        return TRAX_REGION_RECTANGLE;
    }

    if (str == "polygon") {
        return TRAX_REGION_POLYGON;
    }

    if (str == "mask") {
        return TRAX_REGION_MASK;
    }

    MEX_ERROR("Illegal region type");
}

int get_image_code(string str) {

    if (str == "path") {
        return TRAX_IMAGE_PATH;
    }

    if (str == "url") {
        return TRAX_IMAGE_URL;
    }

    if (str == "data") {
        return TRAX_IMAGE_BUFFER;
    }

    if (str == "memory") {
        return TRAX_IMAGE_MEMORY;
    }

    MEX_ERROR("Illegal image type");
}

typedef int(*code_parser)(string name);

int get_flags(const mxArray * input, code_parser parser) {

    if (!mxIsCell(input)) {
        return parser(get_string(input));
    }

    int codes = 0;

    int length = max(mxGetM(input), mxGetN(input));

    if ( min(mxGetM(input), mxGetN(input)) != 1 )
        mexErrMsgTxt("Array must be a vector");

    for (int i = 0; i < length; i++) {
        mxArray* val = mxGetCell (input, i);

        codes |= parser(get_string(val));

    }

    return codes;
}


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

    if (operation == "setup") {

        if ( nrhs != 3 ) { MEX_ERROR("Three parameters required."); return; }

        if ( nlhs > 1 ) { MEX_ERROR("At most one output argument supported."); return; }

        trax_configuration config;
        config.format_region = get_flags(prhs[1], get_region_code);
        config.format_image = get_flags(prhs[2], get_image_code);

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

        if (!getenv("TRAX_SOCKET")) {
            MEX_ERROR("Socket information not available. Enable socket communication in TraX client.");
            return;
        }

#endif

        handle = trax_server_setup(config, trax_no_log);

        if (nlhs == 1)
            plhs[0] = mxCreateLogicalScalar(handle > 0);

    } else if (operation == "wait") {

        if (!handle) { MEX_ERROR("Protocol not initialized."); return; }

        if ( nrhs != 1 ) { MEX_ERROR("No additional parameters required."); return; }

        if ( nlhs > 3 ) { MEX_ERROR("At most three output argument supported."); return; }

        trax_image* img = NULL;
        trax_region* reg = NULL;
        trax_properties* prop = trax_properties_create();

        int tr = trax_server_wait(handle, &img, &reg, prop);

        if (tr == TRAX_INITIALIZE) {

            if (nlhs > 0) plhs[0] = image_to_array(img);
            if (nlhs > 1) plhs[1] = region_to_array(reg);
            if (nlhs > 2) plhs[2] = parameters_to_cell(prop);

        } else if (tr == TRAX_FRAME) {

            if (nlhs > 0) plhs[0] = image_to_array(img);
            if (nlhs > 1) plhs[1] = MEX_CREATE_EMTPY;
            if (nlhs > 2) plhs[2] = parameters_to_cell(prop);

        } else {

            if (nlhs > 0) plhs[0] = MEX_CREATE_EMTPY;
            if (nlhs > 1) plhs[1] = MEX_CREATE_EMTPY;
            if (nlhs > 2) plhs[2] = MEX_CREATE_EMTPY;

            trax_cleanup(&handle);

        }

        if (img) trax_image_release(&img);
        if (reg) trax_region_release(&reg);
        if (prop) trax_properties_release(&prop);

    } else if (operation == "status") {

        if (!handle) { MEX_ERROR("Protocol not initialized."); return; }

        if ( nrhs > 3 ) { MEX_ERROR("Too many parameters."); return; }

        if ( nrhs < 2 ) { MEX_ERROR("Must specify region parameter."); return; }

        if ( nlhs > 1 ) { MEX_ERROR("One output parameter allowed."); return; }

        if (!MEX_TEST_DOUBLE(1)) { MEX_ERROR("Illegal region format."); return; }

        trax_region* reg = array_to_region(prhs[1]);

        if (!reg) { MEX_ERROR("Illegal region format."); return; }

        trax_properties* prop = ( nrhs == 3 ) ? struct_to_parameters(prhs[2]) : NULL;

        trax_server_reply(handle, reg, prop);

        if (reg) trax_region_release(&reg);
        if (prop) trax_properties_release(&prop);

        if (nlhs == 1)
            plhs[0] = mxCreateLogicalScalar(handle > 0);

    } else if (operation == "quit") {

        trax_cleanup(&handle);

    } else {
        MEX_ERROR("Unknown operation.");
    }

}

