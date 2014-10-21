
#include <string.h>
#include <stdio.h>

#include "trax.h"
#include "message.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 


#else

#define strcmpi strcasecmp

#endif

int main( int argc, char** argv) {

    if (argc < 2) return 0;

    if (strcmpi(argv[1], "parsing") == 0) {

        string_list arguments;
        trax_properties* properties;
        FILE* input;

        if (argc != 3) return 0;

        input = fopen(argv[2], "r");

        LIST_CREATE(arguments, 8);
        properties = trax_properties_create();

        while (1) {

            int type;

            LIST_RESET(arguments);
            trax_properties_clear(properties);

            type = read_message(input, NULL, &arguments, properties);

            if (type == TRAX_ERROR) break;

            write_message(stdout, NULL, type, arguments, properties); 

        }

        fclose(input);

        LIST_DESTROY(arguments);
        trax_properties_release(&properties);

    }

}



