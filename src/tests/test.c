
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "trax.h"
#include "message.h"
#include "region.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 


#else

#define strcmpi strcasecmp

#endif

void print_raster(char* raster, int x, int y, int width, int height) {

    int i, j;

    fprintf(stdout, "Raster (origin pixel is x=%d, y=%d) %dx%d:\n", x, y, width, height);

    fputc('o', stdout);
    for (i = 0; i < width; i++) fputc('-', stdout);
    fputc('o', stdout); fputc('\n', stdout);

    for (j = 0; j < height; j++) {
        fputc('|', stdout);
        for (i = 0; i < width; i++) {
            fputc(raster[j * width + i] ? 219 : 32, stdout);
        }
        fprintf(stdout, "|\n");
    }

    fputc('o', stdout);
    for (i = 0; i < width; i++) fputc('-', stdout);
    fputc('o', stdout); fputc('\n', stdout);

}

void print_mask(region_container* region) {

    print_raster(region->data.mask.data, region->data.mask.x, region->data.mask.y, region->data.mask.width, region->data.mask.height);

}

int main( int argc, char** argv) {

    if (argc < 2) return 0;

    if (strcmpi(argv[1], "parsing") == 0) {

        string_list *arguments;
        trax_properties* properties;
        int input;
        int output = fileno(stdout);

        if (argc != 3) return 0;

        input = open(argv[2], O_RDONLY);

        arguments = list_create(8);
        properties = trax_properties_create();

        message_stream* stream = create_message_stream_file(input, output);

        while (1) {

            int type;

            list_reset(arguments);
            trax_properties_clear(properties);

            type = read_message(stream, NULL, arguments, properties);

            if (type == TRAX_ERROR) break;

            write_message(stream, NULL, type, arguments, properties); 

        }

        close(input);

        list_destroy(&arguments);
        trax_properties_release(&properties);

        destroy_message_stream(&stream);

    } else if (strcmpi(argv[1], "overlap") == 0) {

        region_container* a, *b;

        if (argc != 4) return 0;

        region_parse(argv[2], &a);
        region_parse(argv[3], &b);

        fprintf(stdout, "Region A: ");
        region_print(stdout, a);
        fprintf(stdout, "\n");
        fprintf(stdout, "Region B: ");
        region_print(stdout, b);
        fprintf(stdout, "\n");

        float overlap = region_compute_overlap(a, b, region_no_bounds).overlap;

        fprintf(stdout, "Overlap of A and B: %f\n", overlap);

        region_release(&a);
        region_release(&b);
    } else if (strcmpi(argv[1], "polygon") == 0) {

        region_container* a, *b;

        if (argc != 3) return 0;

        region_parse(argv[2], &a);

        b = region_convert(a, POLYGON);

        fprintf(stdout, "Region: ");
        region_print(stdout, a);
        fprintf(stdout, "\n");
        fprintf(stdout, "Polygon: ");
        region_print(stdout, b);
        fprintf(stdout, "\n");

        region_release(&a);
        region_release(&b);
    } else if (strcmpi(argv[1], "raster") == 0) {

        region_container* a;
        char* raster;
        region_bounds bounds;
        int width, height;

        if (argc != 3) return 0;

        region_parse(argv[2], &a);
        bounds = region_compute_bounds(a);
	    bounds.top = floor(bounds.top);
	    bounds.bottom = ceil(bounds.bottom);
	    bounds.left = floor(bounds.left);
	    bounds.right = ceil(bounds.right);

        fprintf(stdout, "Region: ");
        region_print(stdout, a);
        fprintf(stdout, "\n");

        width = (int) (bounds.right - bounds.left + 1);
        height = (int) (bounds.bottom - bounds.top + 1);

        raster = (char*) malloc(sizeof(char) * width * height);
        region_get_mask_offset(a, raster, (int) bounds.left, (int) bounds.top, width, height);

        print_raster(raster, (int) bounds.left, (int) bounds.top, width, height);

        region_release(&a);
        free(raster);
    } else if (strcmpi(argv[1], "mask") == 0) {

        region_container* a, *b, *c;

        if (argc != 3) return 0;

        region_parse(argv[2], &a);

        b = region_convert(a, MASK);
        c = region_convert(b, RECTANGLE);

        fprintf(stdout, "Region: ");
        region_print(stdout, a);
        fprintf(stdout, "\n");
        fprintf(stdout, "Mask: ");
        region_print(stdout, b);
        fprintf(stdout, "\n");
        print_mask(b);        
        fprintf(stdout, "\n");
        fprintf(stdout, "Region: ");
        region_print(stdout, c);
        fprintf(stdout, "\n");

        region_release(&a);
        region_release(&b);
    }
}



