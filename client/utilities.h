#ifndef __UTILITIES_H
#define __UTILITIES_H

#include "trax.h"

typedef struct image_size {
    int width;
    int height;
} image_size;

image_size image_get_size(trax_image* image);

#endif

