#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define TRAX_PREFIX "@@TRAX:"

#define TRAX_STREAM_FILES 1
#define TRAX_STREAM_SOCKET 3

#include <stdio.h>
#include <assert.h>
#include "buffer.h"
#include "trax.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

typedef struct message_stream {
    int flags;
    int input;
    int output;
} message_stream;

#else

typedef struct message_stream {
    int flags;
    int input;
    int output;
} message_stream;

#endif

message_stream* create_message_stream_file(int input, int output);

message_stream* create_message_stream_socket(char* address);

void destroy_message_stream(message_stream** stream);

int read_message(message_stream* stream, FILE* log, string_list* arguments, trax_properties* properties);
	
void write_message(message_stream* stream, FILE* log, int type, const string_list arguments, trax_properties* properties);

#endif
