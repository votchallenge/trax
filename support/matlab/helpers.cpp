
#include <algorithm>

#include "helpers.h"

string get_string(const mxArray *arg) {

    if (mxGetM(arg) != 1) {
        MEX_ERROR("Must be a string");
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

Properties struct_to_parameters(const mxArray * input) {

    if (!mxIsStruct(input)) {
        MEX_ERROR("Parameter has to be a structure.");
        return Properties();
    }

    Properties prop;

    for (int i = 0; i < mxGetNumberOfFields(input); i++) {

        const char* name = mxGetFieldNameByNumber(input, i);

        mxArray * value = mxGetFieldByNumber(input, 0, i);

        if (mxIsChar(value)) {
            string s = get_string(value);
            prop.set(name, s.c_str());
        } else {
            prop.set(name, (float) mxGetScalar(value));
        }
    }

    return prop;
}

Properties cell_to_parameters(const mxArray * input) {

    if (!mxIsCell(input)) {
        MEX_ERROR("Parameter has to be a cell array.");
        return Properties();
    }

    int length = mxGetM(input);
    int n = mxGetN(input);

    if ( min(mxGetM(input), mxGetN(input)) != 1 )
        mexErrMsgTxt("Array must be a vector");


    Properties prop;

    for (int i = 0; i < length; i++) {
        mxArray* key = mxGetCell(input, i);
        mxArray* value = mxGetCell(input, i + length);

        string skey = get_string(key);

        if (mxIsChar(value)) {
            string s = get_string(value);
            prop.set(skey.c_str(), s.c_str());
        } else {
            prop.set(skey.c_str(), (float) mxGetScalar(value));
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

void _param_cell_enumerator(const char *key, const char *value, const void *obj) {

    _param_copy* tmp = (_param_copy*) obj;

    const char** cpkey = (const char **) & (key);
    mxArray* mxKey = mxCreateCharMatrixFromStrings(1, cpkey);

    const char** cpval = (const char **) & (value);
    mxArray* mxVal = mxCreateCharMatrixFromStrings(1, cpval);

    mxSetCell(tmp->array, tmp->pos, mxKey);
    mxSetCell(tmp->array, tmp->pos + tmp->length, mxVal);

    tmp->pos++;

}

mxArray* parameters_to_cell(Properties& input) {

    int length = 0;

    input.enumerate(_param_count_enumerator, &length);

    _param_copy tmp;

    tmp.array = mxCreateCellMatrix(length, 2);
    tmp.pos = 0;
    tmp.length = length;

    input.enumerate(_param_cell_enumerator, &tmp);

    return tmp.array;
}

void _param_names_enumerator(const char *key, const char *value, const void *obj) {

	int i;
    char** fieldnames = (char **) obj;

	fieldnames[0] = (char*)mxMalloc(sizeof(key));

	memcpy(fieldnames[0], key, sizeof(key));

	for (i = 1; i < sizeof(key); i++) {
		if (!isalnum(fieldnames[0][i]) || fieldnames[0][i] != '_')
			fieldnames[0][i] = '_';
	}

    (*fieldnames) ++;
}

void _param_struct_enumerator(const char *key, const char *value, const void *obj) {

    _param_copy* tmp = (_param_copy*) obj;

    const char** cpval = (const char **) & (value);
    mxArray* mxVal = mxCreateCharMatrixFromStrings(1, cpval);

    mxSetFieldByNumber(tmp->array, 0, tmp->pos, mxVal);

    tmp->pos++;

}

mxArray* parameters_to_struct(Properties& input) {

    int length = 0;

    input.enumerate(_param_count_enumerator, &length);

	char **fieldnames = (char **) mxMalloc(sizeof(char*) * length);

	input.enumerate(_param_names_enumerator,fieldnames);

    _param_copy tmp;

    tmp.array = mxCreateStructMatrix(1, 1, length,  (const char **) fieldnames);
    tmp.pos = 0;
    tmp.length = length;

    input.enumerate(_param_struct_enumerator, &tmp);

    return tmp.array;

}

Region array_to_region(const mxArray* input) {

    Region p;
    double *r = (double*)mxGetPr(input);
    int l = mxGetN(input);

    if (l % 2 == 0 && l > 6) {

        p = Region::create_polygon(l / 2);

        for (int i = 0; i < l / 2; i++) {
            p.set_polygon_point(i, r[i * 2], r[i * 2 + 1]);
        }

    } else if (l == 4) {

        p = Region::create_rectangle(r[0], r[1], r[2], r[3]);

    }

    return p;

}

mxArray* region_to_array(const Region& region) {

    mxArray* val = NULL;

    switch (region.type()) {
    case TRAX_REGION_RECTANGLE: {
        val = mxCreateDoubleMatrix(1, 4, mxREAL);
        double *p = (double*) mxGetPr(val); float x, y, width, height;
        region.get(&x, &y, & width, &height);
        p[0] = x; p[1] = y; p[2] = width; p[3] = height;

        break;
    }
    case TRAX_REGION_POLYGON: {
        val = mxCreateDoubleMatrix(1, region.get_polygon_count() * 2, mxREAL);
        double *p = (double*) mxGetPr(val);

        for (int i = 0; i < region.get_polygon_count(); i++) {
            float x, y;
            region.get_polygon_point(i, &x, &y);
            p[i * 2] = x; p[i * 2 + 1] = y;
        }

        break;
    }
    case TRAX_REGION_SPECIAL: {
        val = mxCreateDoubleMatrix(1, 1, mxREAL);
        double *p = (double*) mxGetPr(val);
        p[0] = region.get();
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


mxArray* image_to_array(const Image& img) {

    mxArray* val = NULL;

    switch (img.type()) {
    case TRAX_IMAGE_PATH: {
        const char* cp = img.get_path().c_str();
        val = mxCreateCharMatrixFromStrings(1, &cp);
        break;
    }
    case TRAX_IMAGE_URL: {
        const char* cp = img.get_url().c_str();
        val = mxCreateCharMatrixFromStrings(1, &cp);
        break;
    }
    case TRAX_IMAGE_MEMORY: {
        int width, height;
        int format, depth;
        mwSize dims[3];
        mxClassID classid;
        img.get_memory_header(&width, &height, &format);

        dims[0] = height;
        dims[1] = width;
        switch (format) {
        case TRAX_IMAGE_MEMORY_GRAY8: {
            dims[2] = 1;
            val = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
            char *data = (char*) mxGetPr(val);
            const char* source = img.get_memory_row(0);
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
            const unsigned short* source = (const unsigned short*) img.get_memory_row(0);

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
            const char* source = img.get_memory_row( 0);

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
        const char* source = img.get_buffer(&length, &format);

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

Image array_to_image(const mxArray* arr) {

	int ndims = mxGetNumberOfDimensions(arr);
	const mwSize* dims = mxGetDimensions(arr);

	if (mxIsChar(arr)) {
		if (ndims > 2 || (ndims == 2 && dims[0] != 1))
			MEX_ERROR("Illegal image format, not a valid path or URL");

		string pth = get_string(arr);
		// TODO: determine if path is url

		return Image::create_path(pth);

	}

	if (mxIsUint8(arr)) {

		if (ndims > 3)
			MEX_ERROR("Illegal image format, unsupported size");

		if (ndims == 3 && !(dims[2] == 1 || dims[2] == 3))
			MEX_ERROR("Illegal image format, unsupported image channel count");

		int channels = ndims < 3 ? 1 : dims[2];
		int width = ndims < 2 ? 1 : dims[1];
		int height = dims[0];

		if (channels == 3) {
			Image img = Image::create_memory(width, height, TRAX_IMAGE_MEMORY_RGB);
            const char *data = (const char*) mxGetPr(arr);
            char* dest = img.write_memory_row(0);

            for (int i = 0; i < width; i++) {
                for (int j = 0; j < height; j++) {
                    for (int k = 0; k < 3; k++) {
                        dest[3 * (i + j * width) + k] = data[j + height * (i + k * width)];
                    }
                }
            }
			return img;
		}

		Image img = Image::create_memory(width, height, TRAX_IMAGE_MEMORY_GRAY8);
        const char *data = (const char*) mxGetPr(arr);
        char* dest = img.write_memory_row(0);

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                dest[i + j * width] = data[j + height * i];
            }
        }
		return img;

	}

	if (mxIsUint16(arr)) {

		if (ndims > 2)
			MEX_ERROR("Illegal image format, unsupported size");

		int width = ndims < 2 ? 1 : dims[1];
		int height = dims[0];

        Image img = Image::create_memory(width, height, TRAX_IMAGE_MEMORY_GRAY16);
        const unsigned short *data = (const unsigned short*) mxGetPr(arr);
        unsigned short* dest = (unsigned short*) img.write_memory_row(0);

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                dest[i + j * width] = data[j + height * i];
            }
        }

		return img;
	}

	MEX_ERROR("Unable to determine image type");

}


int get_argument_code(string str) {

	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str == "propstruct") {
        return ARGUMENT_PROPSTRUCT;
    }

    if (str == "name") {
        return ARGUMENT_TRACKERNAME;
    }

    if (str == "version") {
        return ARGUMENT_TRACKERVERSION;
    }

    if (str == "family") {
        return ARGUMENT_TRACKERFAMILY;
    }

    if (str == "debug") {
        return ARGUMENT_DEBUG;
    }

    if (str == "timeout") {
        return ARGUMENT_TIMEOUT;
    }

    if (str == "environment") {
        return ARGUMENT_ENVIRONMENT;
    }

    if (str == "data") {
        return ARGUMENT_STATEOBJECT;
    }

    if (str == "connection") {
        return ARGUMENT_CONNECTION;
    }

    if (str == "directory") {
        return ARGUMENT_DIRECTORY;
    }

    if (str == "log") {
        return ARGUMENT_LOG;
    }

    MEX_ERROR("Illegal argument name");
}

int get_region_code(string str) {

	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

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

	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str == "path") {
        return TRAX_IMAGE_PATH;
    }

    if (str == "url") {
        return TRAX_IMAGE_URL;
    }

    if (str == "buffer") {
        return TRAX_IMAGE_BUFFER;
    }

    if (str == "memory") {
        return TRAX_IMAGE_MEMORY;
    }

    MEX_ERROR("Illegal image type");
}

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

