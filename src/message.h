#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define TRAX_PREFIX "@@TRAX:"

#define TRAX_STREAM_FILES 1
#define TRAX_STREAM_SOCKET 2
#define TRAX_STREAM_SOCKET_LISTEN 8
#define TRAX_STREAM_ASYNC 16

#define TRAX_BUFFER_SIZE 4096

#include <stdio.h>
#include <assert.h>
#include "buffer.h"
#include "trax.h"

typedef struct socket_data {
    int server;
    int socket;
} socket_data;

typedef struct files_data {
    int input;
    int output;
} files_data;

typedef struct input_cache {
    int message_type;
    int complete;
    int state;
    string_buffer* key_buffer, *value_buffer;
} input_cache;

typedef struct output_cache {
    int complete;
    int state;
    string_buffer* key_buffer, *value_buffer;
} output_cache;


typedef struct message_stream {
    int flags;
    union {
        files_data files;
        socket_data socket;
    };
    char buffer[TRAX_BUFFER_SIZE];
    int buffer_position;
    int buffer_length;
    input_cache input;
    output_cache output;
} message_stream;

message_stream* create_message_stream_file(int input, int output);

message_stream* create_message_stream_socket_connect(int port);

message_stream* create_message_stream_socket_accept(int server, int timeout);

void destroy_message_stream(message_stream** stream);

int read_message(message_stream* stream, trax_logging* log, string_list* arguments, trax_properties* properties);
	
void write_message(message_stream* stream, trax_logging* log, int type, const string_list* arguments, trax_properties* properties);

#define LOG_STRING(L, S) { if ((L) && (L)->callback ) { (L)->callback(S, strlen(S), (L)->data); } }
#define LOG_BUFFER(L, S, N) { if ((L) && (L)->callback ) { (L)->callback(S, N, (L)->data); } }
#define LOG_CHAR(L, C) { if ((L) && (L)->callback ) { (L)->callback(&(C), 1, (L)->data); } }

#endif
