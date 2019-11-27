
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "region.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 


#else

#include <unistd.h>
#define strcmpi strcasecmp

#endif

const char* test_strings[] = {
    "10.0000,10.0000,10.0000,10.0000",
    "10.0000,10.0000,20.0000,20.0000,30.0000,10.0000",
    "10.0000,10.0000,20.0000,10.0000,20.0000,20.0000,10.0000,20.0000",
    "mask:10,10,30,30,5,10,5,10,5,1,20,30,2,30,2,30,30,30,2,30,5,7,23,2",
    (char*) 0
};

int main( int argc, char** argv) {

    int t = 0;

    while (1) {

        if (!test_strings[t]) break;

        {
            const char* source = test_strings[t];
            char* a, *b;
            region_container *r1, *r2;

            region_parse(source, &r1);

            a = region_string(r1);

            region_parse(a, &r2);

            b = region_string(r2);
    
            printf("%s ** %s ** %s \n", source, a, b);

            assert(strcmpi(source, a) == 0 && strcmpi(b, a) == 0);

        }

        t++;

    }

}



