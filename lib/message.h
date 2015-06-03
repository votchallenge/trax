#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define TRAX_PREFIX "@@TRAX:"

#include <stdio.h>
#include <assert.h>
#include "buffer.h"
#include "trax.h"

typedef struct socket_data {
    int server;
    int socket;
    int port;
} socket_data;

typedef struct files_data {
    int input;
    int output;
} files_data;

typedef struct message_stream {
    int flags;
    union {
        files_data files;
        socket_data socket;
    };
} message_stream;

message_stream* create_message_stream_file(int input, int output);

message_stream* create_message_stream_socket_connect(int port);

message_stream* create_message_stream_socket_listen();

void destroy_message_stream(message_stream** stream);

int get_socket_port(message_stream* stream);

int read_message(message_stream* stream, FILE* log, string_list* arguments, trax_properties* properties);
	
void write_message(message_stream* stream, FILE* log, int type, const string_list arguments, trax_properties* properties);

#endif
