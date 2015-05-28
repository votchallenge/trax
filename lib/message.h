#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdio.h>
#include "buffer.h"
#include "trax.h"

#define TRAX_PREFIX "@@TRAX:"

int read_message(int input, int log, string_list* arguments, trax_properties* properties);
	
void write_message(int output, int log, int type, const string_list arguments, trax_properties* properties);

#endif
