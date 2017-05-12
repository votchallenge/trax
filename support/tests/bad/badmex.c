#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "mex.h"

#ifndef OCTAVE
#include "matrix.h"
#endif

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	int type = 0;

    if ( nrhs > 0 ) {
		char* ct = mxArrayToString(prhs[0]);
		if (strcmp(ct, "sigabrt") == 0)
			type = 1;
		else if (strcmp(ct, "sigsegv") == 0)
			type = 2;
		else if (strcmp(ct, "sigusr1") == 0)
			type = 3;

		mxFree(ct);
	}

	switch (type) {
	case 1:
		raise (SIGABRT);
	case 2:
		raise (SIGSEGV);
	case 3:
		raise (SIGUSR1);
	default:
		abort();
	}

}
