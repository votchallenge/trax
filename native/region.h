/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */


#ifndef _REGION_H_
#define _REGION_H_

#include <stdlib.h>
#include <string.h>

#include "trax.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

#ifdef __cplusplus
extern "C" {
#endif

int parse_region(char* buffer, trax_region** region);

char* string_region(trax_region* region);

void print_region(FILE* out, trax_region* region);

trax_region* convert_region(const trax_region* region, int type);

#ifdef __cplusplus
}
#endif

#endif
