
#ifndef __TRAX_MEX_HELPERS
#define __TRAX_MEX_HELPERS

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <trax.h>

#include "mex.h"

#ifndef OCTAVE
#include "matrix.h"
#endif

using namespace std;
using namespace trax;

#define MEX_TEST_DOUBLE(I) (mxGetClassID(prhs[I]) == mxDOUBLE_CLASS)
#define MEX_TEST_VECTOR(I) (mxGetNumberOfDimensions(prhs[I]) == 2 && mxGetM(prhs[I]) == 1)
#define MEX_CREATE_EMTPY mxCreateDoubleMatrix(0, 0, mxREAL)
#define MEX_ERROR(M) mexErrMsgTxt(M)

#define ARGUMENT_CUSTOM -1
#define ARGUMENT_PROPSTRUCT 0
#define ARGUMENT_TRACKERNAME 1
#define ARGUMENT_TRACKERFAMILY 2
#define ARGUMENT_TRACKERDESCRIPTION 3
#define ARGUMENT_DEBUG 4
#define ARGUMENT_TIMEOUT 5
#define ARGUMENT_ENVIRONMENT 6
#define ARGUMENT_STATEOBJECT 7
#define ARGUMENT_CONNECTION 8
#define ARGUMENT_DIRECTORY 9
#define ARGUMENT_LOG 10
#define ARGUMENT_CHANNELS 11

bool is_property_name(const std::string &str);

string get_string(const mxArray *arg);

mxArray* set_string(const char* str);

Properties struct_to_parameters(const mxArray * input);

Properties cell_to_parameters(const mxArray * input);

mxArray* parameters_to_cell(Properties& input);

mxArray* parameters_to_struct(Properties& input);

Region array_to_region(const mxArray* input);

mxArray* region_to_array(const Region& region);

mxArray* image_to_array(const Image& img);

mxArray* images_to_array(const ImageList& img);

Image array_to_image(const mxArray* arr);

ImageList array_to_images(const mxArray* arr, int channels);

int get_argument_code(string str);

int get_region_code(string str);

int get_image_code(string str);

int get_channel_code(string str);

typedef int(*code_parser)(string name);

int get_flags(const mxArray* input, code_parser parser);

mxArray* decode_region(int formats);

mxArray* decode_image(int formats);

mxArray* decode_channels(int channels);

#endif

