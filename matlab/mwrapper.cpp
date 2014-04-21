#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <trax.h>

#include "mex.h"
#include "matrix.h"
#include "engine.h"

#ifdef __GNUC__

#include <getopt.h>

#else

#define EOF	(-1)

#define ERR(s, c)	if(opterr){\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	fputs(argv[0], stderr);\
	fputs(s, stderr);\
	fputc(c, stderr);}

extern "C" {

extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;
extern int getopt(int argc, char **argv, char *opts);

}


int	opterr = 1;
int	optind = 1;
int	optopt;

char	*optarg;

int getopt(int argc, char **argv, char *opts)
{

	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {

			optind++;
			return(EOF);

		}

	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == 0) {
		ERR(": illegal option -- ", c);

		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;

		}

		return('?');

	}

	if(*++cp == ':') {

		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');

		} else
			optarg = argv[optind++];
		sp = 1;

	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}

		optarg = NULL;

	}
	return(c);

}

#endif 

using namespace std;

string string_replace( string src, string const& target, string const& repl)
{
    // handle error situations/trivial cases

    if (target.length() == 0) {
        // searching for a match to the empty string will result in 
        //  an infinite loop
        //  it might make sense to throw an exception for this case
        return src;
    }

    if (src.length() == 0) {
        return src;  // nothing to match against
    }

    size_t idx = 0;

    for (;;) {
        idx = src.find( target, idx);
        if (idx == string::npos)  break;

        src.replace( idx, target.length(), repl);
        idx += repl.length();
    }

    return src;
}

trax_region* array_to_region(const mxArray* input) {

    trax_region* p = NULL;
	double *r = (double*)mxGetPr(input);
    int l = mxGetN(input);
    
    if (l % 2 == 0 && l > 6) {
        
        p = trax_region_create_polygon(l / 2);
        
        for (int i = 0; i < l / 2; i++) {
            trax_region_set_polygon_point(p, i, r[i*2], r[i*2+1]);
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
			p[i*2] = x; p[i*2+1] = y;
		}

		break;
	}
	case TRAX_REGION_SPECIAL: {
		val = mxCreateDoubleMatrix(1, 1, mxREAL);
		double *p = (double*) mxGetPr(val);
		p[0] = trax_region_get_special(region); 
		break;
	}
	}

	return val;
}

#define BUFFERSIZE 1024

bool executeCommand(Engine *ep, const string& command, bool echo) {
	char buffer[BUFFERSIZE];

	if (echo) {
		fprintf(stdout, "Running: %s\n", command.c_str());
	} 

	engOutputBuffer(ep, buffer, BUFFERSIZE);

	string wrapped = string("try \n") + command + string("\n e__ = []; catch e \n e__ = e; \n end");

	engEvalString(ep, wrapped.c_str());

	if (echo) {
		fprintf(stdout, "%s\n", buffer);
	} 

	bool failed = false;

	mxArray *err = engGetVariable(ep, "e__");
    if(mxIsStruct(err)) {

		mxArray *errStr = mxGetField(err,0,"message");
		if( (errStr != NULL) && mxIsChar(errStr) )
		{
			char str[512];
			mxGetString(errStr,str,sizeof(str)/sizeof(str[0]));
			if (echo) fprintf(stdout, "%s\n", str);
		}
		failed = true;
	}

	mxDestroyArray(err);

	return !failed;
}

void print_help() {

}

#define CMD_OPTIONS "p:I:U:hdr:i:l"

int main(int argc, char** argv) {

	Engine *ep;
	
	bool loadImages = true;
	bool debug = false;
	string initCommand;
	string updateCommand;


	vector<string> paths;

	if (argc < 2) {
		fprintf(stderr, "TraX MATLAB wrapper\n");
		fprintf(stderr, "Usage: mwrapper -h -d -p <path> -I <initialize_command> -U <update_command>\n");
		return EXIT_FAILURE;
	}

	int c;
    while ((c = getopt(argc, argv, CMD_OPTIONS)) != -1)
        switch (c) {
        case 'h':
            print_help();
            exit(0);
        case 'd':
            debug = true;
            break;
        case 'I':
            initCommand = string(optarg);
            break;
        case 'U':
            updateCommand = string(optarg);
            break;
        case 'p':
            paths.push_back(string(optarg));
            break;
        default: 
            print_help();
            throw std::runtime_error(string("Unknown switch -") + string(1, (char) optopt));
        } 

    if (optind < argc) {
        print_help();
        exit(-1);
    }

	string matlab_executable("");

#ifndef WIN32
	if (!getenv("MATLAB_ROOT")) {
		fprintf(stderr, "MATLAB_ROOT not defined as an environmental variable (this is required on Posix systems)\n");
		return EXIT_FAILURE;
	}
	matlab_executable = string(getenv("MATLAB_ROOT")) + string("/bin/matlab -nodesktop -nosplash");
#endif

	if (!(ep = engOpen(matlab_executable.c_str()))) {
		fprintf(stderr, "Can't start MATLAB engine\n");
		return EXIT_FAILURE;
	}

#ifdef WIN32
	engSetVisible(ep, 0);
#endif

	/* Add paths */
	for (int i = 0; i < paths.size(); i++) {
		string command = string("addpath('") + string_replace(paths[i], "'", "''") + string("');");
		executeCommand(ep, command, debug);
	}

    trax_image* img = NULL;
    trax_region* reg = NULL;
    trax_region* mem = NULL;

    trax_handle* trax;
    trax_configuration config;
    config.format_region = TRAX_REGION_RECTANGLE;
    config.format_image = TRAX_IMAGE_PATH;

	trax = trax_server_setup_standard(config, NULL);
	
    int run = 1;

	if (loadImages) {
		initCommand = string("image = imread(image); ") + initCommand;
		updateCommand = string("image = imread(image); ") + updateCommand;
	}

    while(run)
    {

        int tr = trax_server_wait(trax, &img, &reg, NULL);

        if (tr == TRAX_INITIALIZE) {

			mxArray* region = region_to_array(reg);
			if (!region) { run = 0; continue; }

			const char** cp = (const char **) &(img->data);
			mxArray* path = mxCreateCharMatrixFromStrings(1, cp);

			engPutVariable(ep, "image", path);
			engPutVariable(ep, "region", region);

			mxDestroyArray(path);
			mxDestroyArray(region);

			if (executeCommand(ep, initCommand, debug)) {

				region = engGetVariable(ep, "region");
				trax_region* regout = array_to_region(region);
				if (regout) 
					trax_server_reply(trax, regout, NULL);
				else {
					fprintf(stderr, "Region not available, unable to continue\n");
					run = 0;
				}
				if (regout) trax_region_release(&regout);
				if (region) mxDestroyArray(region);
			
			} else {

				run = 0;

			}

			

        } else if (tr == TRAX_FRAME) {

			const char** cp = (const char **) &(img->data);
			mxArray* path = mxCreateCharMatrixFromStrings(1, cp);

			engPutVariable(ep, "image", path);
			mxDestroyArray(path);

			if (executeCommand(ep, updateCommand, debug)) {

				mxArray* region = engGetVariable(ep, "region");

				trax_region* regout = array_to_region(region);
				if (regout) 
					trax_server_reply(trax, regout, NULL);
				else {
					fprintf(stderr, "Region not available, unable to continue\n");
					run = 0;
				}
				if (regout) trax_region_release(&regout);

				if (region) mxDestroyArray(region);
			} else {
				run = 0;
				
			}

			

        } 
        else {

            run = 0;

        }

        if (img) trax_image_release(&img);
        if (reg) trax_region_release(&reg);

    }

    if (img) trax_image_release(&img);
    if (reg) trax_region_release(&reg);
    if (mem) trax_region_release(&mem);

    trax_cleanup(&trax);

	engClose(ep);
	
	return EXIT_SUCCESS;
}

