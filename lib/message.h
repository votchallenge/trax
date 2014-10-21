#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdio.h>
#include "buffer.h"
#include "trax.h"

#define TRAX_PREFIX "@@TRAX:"

int read_message(FILE* input, FILE* log, string_list* arguments, trax_properties* properties);
	
void write_message(FILE* output, FILE* log, int type, const string_list arguments, trax_properties* properties);

#endif
