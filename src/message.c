/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
#include "message.h"
#include "debug.h"

#define PARSE_STATE_TYPE 0
#define PARSE_STATE_SPACE_EXPECT 1
#define PARSE_STATE_SPACE 2
#define PARSE_STATE_UNQUOTED_KEY 3
#define PARSE_STATE_UNQUOTED_VALUE 4
#define PARSE_STATE_UNQUOTED_ESCAPE_KEY 5
#define PARSE_STATE_UNQUOTED_ESCAPE_VALUE 6
#define PARSE_STATE_QUOTED_KEY 7
#define PARSE_STATE_QUOTED_VALUE 8
#define PARSE_STATE_QUOTED_ESCAPE_KEY 9
#define PARSE_STATE_QUOTED_ESCAPE_VALUE 10
#define PARSE_STATE_PASS 100

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include <ctype.h>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#if defined(_MSC_VER)
#include <io.h>
#endif

#define strcmpi _strcmpi

static int initialized = 0;
static void initialize_sockets(void) {
    WSADATA data;
    WORD version = 0x101;
	if (initialized) return;

    WSAStartup(version,&data);
    initialized = 1;
    return;
}

#else

#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#define closesocket close

#define strcmpi strcasecmp

static void initialize_sockets(void) {}

#endif

#define VALIDATE_MESSAGE_STREAM(S) assert((S->flags & TRAX_STREAM_FILES) || (S->flags & TRAX_STREAM_SOCKET))

#define MAX_KEY_LENGTH 64

const int prefix_length = sizeof(TRAX_PREFIX) - 1;

int __is_valid_key(char* c, int len) {
    int i;

    if (len > MAX_KEY_LENGTH) return 0;

    for (i = 0; i < len; i++) {
        if (!(isalnum(c[i]) || c[i] == '.' || c[i] == '_')) return 0;
    }

    return 1;
}

int __parse_message_type(const char *str) {
    
    if (strcmpi(str, "hello") == 0) return TRAX_HELLO;
    if (strcmpi(str, "initialize") == 0) return TRAX_INITIALIZE;
    if (strcmpi(str, "state") == 0) return TRAX_STATE; // Compatibility reasons
    if (strcmpi(str, "status") == 0) return TRAX_STATE;
    if (strcmpi(str, "frame") == 0) return TRAX_FRAME;
    if (strcmpi(str, "quit") == 0) return TRAX_QUIT;

    return -1;
}

void initialize_cache(message_stream* stream) {

    stream->input.message_type = -1;
    stream->input.complete = FALSE;
    stream->input.state = -prefix_length;

    stream->input.key_buffer = buffer_create(BUFFER_INCREMENT_STEP);
    stream->input.value_buffer = buffer_create(BUFFER_INCREMENT_STEP);

}

void destroy_cache(message_stream* stream) {

    buffer_destroy(&(stream->input.key_buffer));
    buffer_destroy(&(stream->input.value_buffer));

}


message_stream* create_message_stream_file(int input, int output) {
    message_stream* stream = (message_stream*) malloc(sizeof(message_stream));

    stream->flags = TRAX_STREAM_FILES;
    stream->files.input = input;
    stream->files.output = output;
    stream->buffer_position = 0;
    stream->buffer_length = 0;

    initialize_cache(stream);

    return stream;
}

message_stream* create_message_stream_socket_connect(int port) {

    message_stream* stream = NULL;

	int sid;
    int one = 1;
	struct sockaddr_in pin;

    initialize_sockets();

	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_port = htons(port);
	pin.sin_addr.s_addr = inet_addr(TRAX_LOCALHOST);

    if((sid = (int)socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    return NULL;
    }
	
	if (setsockopt(sid, IPPROTO_TCP , TCP_NODELAY,
						 (const char *)&one, sizeof(int)) == -1) {
        perror("nodelay");
        closesocket(sid);
    }

	while (1) {

	    if (connect(sid, (const struct sockaddr *)&pin, sizeof(pin))) {
            perror("connect"); // Wait a bit for connection ...
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
            Sleep(1000);
#else
            sleep(1);
#endif
		    continue;
	    }

        break;
    }

    stream = (message_stream*) malloc(sizeof(message_stream));

    stream->flags = TRAX_STREAM_SOCKET;
    stream->socket.server = -1;
    stream->socket.socket = sid;

    stream->buffer_position = 0;
    stream->buffer_length = 0;

    initialize_cache(stream);

    return stream;

}

message_stream* create_message_stream_socket_accept(int server, int timeout) {

	fd_set readfds,writefds,exceptfds;
	int asock=-1;
    int one = 1;
    message_stream* stream = NULL;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

	initialize_sockets();
 
	if(listen(server, 1)== -1) {
		perror("listen");
		return NULL;
	}

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(server,&readfds);
	FD_SET(server,&exceptfds);

    select(server+1,&readfds,&writefds,&exceptfds,&tv);
	
	if(FD_ISSET(server,&readfds)) {
		struct sockaddr_in pin;
		int addrlen = sizeof(struct sockaddr_in);
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
		asock = (int) accept(server,(struct sockaddr *)&pin,
											 (int *)&addrlen);
#else
		asock = (int) accept(server,(struct sockaddr *)&pin,
												 (socklen_t *)&addrlen);
#endif
	} else {
        return NULL;
    }

	if (setsockopt(asock, IPPROTO_TCP , TCP_NODELAY,
						 (const char *)&one, sizeof(int)) == -1) {
        perror("nodelay");
        return NULL;
    }


    stream = (message_stream*) malloc(sizeof(message_stream));

    stream->flags = TRAX_STREAM_SOCKET;
    stream->flags |= TRAX_STREAM_SOCKET_LISTEN;
    stream->socket.server = server;
    stream->socket.socket = asock;
    stream->buffer_position = 0;
    stream->buffer_length = 0;

    initialize_cache(stream);

    return stream;
}

void destroy_message_stream(message_stream** stream) {

    VALIDATE_MESSAGE_STREAM((*stream));

    if ((*stream)->flags & TRAX_STREAM_SOCKET) {

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER) 
	    shutdown((*stream)->socket.socket, SD_BOTH);
#else
	    shutdown((*stream)->socket.socket, SHUT_RDWR);
#endif
        closesocket((*stream)->socket.socket);

    }
    (*stream)->flags = 0;
    
    destroy_cache(*stream);

    free(*stream);
    *stream = 0;

}

static __INLINE int read_character(message_stream* stream) {
    char chr;

    if (stream->buffer_position >= stream->buffer_length) {

        if (stream->flags & TRAX_STREAM_SOCKET) {

		    stream->buffer_length = recv(stream->socket.socket, stream->buffer, TRAX_BUFFER_SIZE, 0);

        } else {

        	stream->buffer_length = read(stream->files.input, stream->buffer, TRAX_BUFFER_SIZE);
           
        }

        if (stream->buffer_length < 0) {
            return -1; // An error has occured  
        } 

		if (stream->buffer_length == 0) {
            return -1; // The stream was closed
        }

       // fprintf(stderr, "RCV: %*s \n", stream->buffer_length-1, stream->buffer);
       // fflush(stderr);
        stream->buffer_position = 0;
    }

    chr = stream->buffer[stream->buffer_position];

    stream->buffer_position++;

    return chr;

}

int read_message(message_stream* stream, trax_logging* log, string_list* arguments, trax_properties* properties) {
	
	VALIDATE_MESSAGE_STREAM(stream);

    list_reset(arguments);

    while (!stream->input.complete) {

    	char chr; 
    	int val = read_character(stream);

    	if (val < 0) {
    		if (stream->input.message_type == -1) break;
    		chr = '\n';
    		stream->input.complete = TRUE;
    	} else chr = (char) val;

        LOG_CHAR(log, chr);

        switch (stream->input.state) {
            case PARSE_STATE_TYPE: { // Parsing message type

                if (isalnum(chr)) {

                    buffer_push(stream->input.key_buffer, chr);

                } else if (chr == ' ') {
                    char* key_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);
                	stream->input.message_type = __parse_message_type(key_tmp);
                    free(key_tmp);
                    
                    if (stream->input.message_type == -1) {
                		stream->input.state = PARSE_STATE_PASS;
                		stream->input.message_type = -1;
                    } else {
                        stream->input.state = PARSE_STATE_SPACE; 
                    }

                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);

                } else if (chr == '\n') {
                    char* key_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);
                	stream->input.message_type = __parse_message_type(key_tmp);
                    free(key_tmp);

                    if (stream->input.message_type == -1) {
                		stream->input.state = PARSE_STATE_PASS;
                		stream->input.message_type = -1;
                    } else {
                        stream->input.complete = TRUE;
                    }

                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                    
                } else {
                   stream->input. state = PARSE_STATE_PASS;
                    buffer_reset(stream->input.key_buffer);
                }

                break;
            }
            case PARSE_STATE_SPACE_EXPECT: {

                if (chr == ' ') {
                    stream->input.state = PARSE_STATE_SPACE;
                } else if (chr == '\n') {
                	stream->input.complete = TRUE;
                } else {
                	stream->input.message_type = -1;
                	stream->input.state = PARSE_STATE_PASS;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                    buffer_push(stream->input.key_buffer, chr);             	
                }

                break;

            }		            
            case PARSE_STATE_SPACE: {

              if (chr == ' ' || chr == '\r') {
                    // Do nothing
                } else if (chr == '\n') {
                	stream->input.complete = TRUE;
                } else if (chr == '"') {
                    stream->input.state = PARSE_STATE_QUOTED_KEY;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                } else {
                	stream->input.state = PARSE_STATE_UNQUOTED_KEY;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                    buffer_push(stream->input.key_buffer, chr);              	
                }

                break;

            }	            
            case PARSE_STATE_UNQUOTED_KEY: {

                if (chr == '\\') {
                    stream->input.state = PARSE_STATE_UNQUOTED_ESCAPE_KEY;
                } else if (chr == '\n') { // append arg and finalize
                    char* key_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);            
                    list_append(arguments, key_tmp);
                    free(key_tmp);

                    stream->input.complete = TRUE;
                } else if (chr == ' ') { // append arg and move on
                    char* key_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);            
                    list_append(arguments, key_tmp);
                    free(key_tmp);

                    stream->input.state = PARSE_STATE_SPACE;
                    buffer_reset(stream->input.key_buffer);

                } else if (chr == '=') { // we have a kwarg
                	if (__is_valid_key(stream->input.key_buffer->buffer, stream->input.key_buffer->position))
                    	stream->input.state = PARSE_STATE_UNQUOTED_VALUE;
                	else { // If the key is not valid then we probably have a Base64 encoding
                		buffer_push(stream->input.key_buffer, chr);
                	}
                } else {                    
                	buffer_push(stream->input.key_buffer, chr);
                } 

                break;

            }
            case PARSE_STATE_UNQUOTED_VALUE: {

                if (chr == '\\') {
                    stream->input.state = PARSE_STATE_UNQUOTED_ESCAPE_VALUE;
                } else if (chr == ' ') {
                    char *key_tmp, *value_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);
                    value_tmp = buffer_extract(stream->input.value_buffer);                
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    stream->input.state = PARSE_STATE_SPACE;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);  
       
                } else if (chr == '\n') {
                    char *key_tmp, *value_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);
                    value_tmp = buffer_extract(stream->input.value_buffer);  
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    stream->input.complete = TRUE;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);

                } else
                    buffer_push(stream->input.value_buffer, chr);

                break;

            }
            case PARSE_STATE_UNQUOTED_ESCAPE_KEY: {

                if (chr == 'n') {
                	buffer_push(stream->input.key_buffer, '\n');
                    stream->input.state = PARSE_STATE_UNQUOTED_KEY;
                } else if (chr != '\n') {
                	buffer_push(stream->input.key_buffer, chr);
                    stream->input.state = PARSE_STATE_UNQUOTED_KEY;
                } else {
                    stream->input.state = PARSE_STATE_PASS;
                    stream->input.message_type = -1;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);

                }
                
                break;

            }	            
            case PARSE_STATE_UNQUOTED_ESCAPE_VALUE: {

                if (chr == 'n') {
                    buffer_push(stream->input.value_buffer, '\n');
                    stream->input.state = PARSE_STATE_UNQUOTED_VALUE;
                } else if (chr != '\n') {
                    buffer_push(stream->input.value_buffer, chr);
                    stream->input.state = PARSE_STATE_UNQUOTED_VALUE;
                } else {
                    stream->input.state = PARSE_STATE_PASS;
                    stream->input.message_type = -1;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                }
                
                break;

            }

            case PARSE_STATE_QUOTED_KEY: {

                if (chr == '\\') {
                    stream->input.state = PARSE_STATE_QUOTED_ESCAPE_KEY;
                } else if (chr == '"') { // append arg and move on
                    char *key_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);            
                    list_append(arguments, key_tmp);
                    free(key_tmp);

                	stream->input.state = PARSE_STATE_SPACE_EXPECT;
                } else if (chr == '=') { // we have a kwarg
                	if (__is_valid_key(stream->input.key_buffer->buffer, stream->input.key_buffer->position))
                    	stream->input.state = PARSE_STATE_QUOTED_VALUE;
                	else { // If the key is not valid then we probably have a Base64 encoding
                		buffer_push(stream->input.key_buffer, chr);
                	}
                } else {                    
                	buffer_push(stream->input.key_buffer, chr);
                } 

                break;

            }
            case PARSE_STATE_QUOTED_VALUE: {

                if (chr == '\\') {
                    stream->input.state = PARSE_STATE_QUOTED_ESCAPE_VALUE;
                } else if (chr == '"') {
                    char *key_tmp, *value_tmp;
                    key_tmp = buffer_extract(stream->input.key_buffer);
                    value_tmp = buffer_extract(stream->input.value_buffer);                
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    stream->input.state = PARSE_STATE_SPACE_EXPECT;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);

                } else
                    buffer_push(stream->input.value_buffer, chr);

                break;

            }
            case PARSE_STATE_QUOTED_ESCAPE_KEY: {

                if (chr == 'n') {
                	buffer_push(stream->input.key_buffer, '\n');
                    stream->input.state = PARSE_STATE_QUOTED_KEY;
                } else if (chr != '\n') {
                	buffer_push(stream->input.key_buffer, chr);
                    stream->input.state = PARSE_STATE_QUOTED_KEY;
                } else {
                    stream->input.state = PARSE_STATE_PASS;
                    stream->input.message_type = -1;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);

                }
                
                break;

            }		            
            case PARSE_STATE_QUOTED_ESCAPE_VALUE: {

                if (chr == 'n') {
                    buffer_push(stream->input.value_buffer, '\n');
                    stream->input.state = PARSE_STATE_QUOTED_VALUE;
                } else if (chr != '\n') {
                    buffer_push(stream->input.value_buffer, chr);
                    stream->input.state = PARSE_STATE_QUOTED_VALUE;
                } else {
                    stream->input.state = PARSE_STATE_PASS;
                    stream->input.message_type = -1;
                    buffer_reset(stream->input.key_buffer);
                    buffer_reset(stream->input.value_buffer);
                }
                
                break;

            }	            
            
            case PARSE_STATE_PASS: {

                if (chr == '\n') 
                    stream->input.state = -prefix_length;

                break;
            }
            default: { // Parsing prefix

                if (stream->input.state < 0) {
                    if (chr == TRAX_PREFIX[prefix_length + stream->input.state])
                    	// When done, go to type parsing
                        stream->input.state++; 
                    else 
                    	// Not a message
                        stream->input.state = chr == '\n' ? -prefix_length : PARSE_STATE_PASS; 
                }

                break;
            }
        }

    }

    LOG_BUFFER(log, NULL, 0) // Flush the log stream

    stream->input.state = -prefix_length;
    stream->input.complete = FALSE;
    buffer_reset(stream->input.key_buffer);
    buffer_reset(stream->input.value_buffer);

    return stream->input.message_type;
    
}

int write_buffer(message_stream* stream, const char* buf, int len, trax_logging* log) {
    if (len < 1) return 1;

    if (stream->flags & TRAX_STREAM_SOCKET) {

        int cnt = 0;

        while(cnt < len) {
            #ifdef MSG_NOSIGNAL
            int l = send(stream->socket.socket, buf+cnt, len-cnt, MSG_NOSIGNAL);
            #else
            int l = send(stream->socket.socket, buf+cnt, len-cnt, 0);
            #endif
            if(l == -1) {
                return -1;
            }
            cnt += l;
        }

    } else {

        int cnt = 0;

        while(cnt < len) {
            int l = write(stream->files.output, buf+cnt, len-cnt);
            if(l == -1) {
                return -1;
            }
            cnt += l;

        }

    }

    LOG_BUFFER(log, buf, len);

    return 1;
}

int write_buffer_escaped(message_stream* stream, const char* buf, int len, trax_logging* log) {
    int i = 0, j = 0;

    if (len < 1) return 1;

    while (i < len) {
        // Handle special characters by writting the currenlty scanned part of
        // the buffer and inserting escape character.
        if (buf[i] == '"' || buf[i] == '\\' || buf[i] == '\n') {
            if (write_buffer(stream, &(buf[j]), i - j, log) == -1)
                return -1;
            j = i;
            if (write_buffer(stream, "\\", 1, log) != 1)
                return -1;
            // In case of new line we do not insert that symbol but character 'n'.
            if (buf[i] == '\n') {
                if (write_buffer(stream, "n", 1, log) != 1)
                    return -1;
                j++;
            }

        }
        i++;
    }
    if (write_buffer(stream, &(buf[j]), i - j, log) == -1)
        return -1;

    return 1;
}

typedef struct file_pair {
    message_stream* stream;
    trax_logging* log;
} file_pair;

#define OUTPUT_STRING(S) { int len = strlen(S); write_buffer(stream, S, len, log); }
#define OUTPUT_ESCAPED(S) { int len = strlen(S); write_buffer_escaped(stream, S, len, log); }
/*#define OUTPUT_ESCAPED(S) { int i = 0; while (1) { \
    if (!S[i]) break; \
    if (S[i] == '"' || S[i] == '\\') { write_string(stream, "\\", 1); LOG_STR(log, "\\"); } \
     write_string(stream, &(S[i]), 1); LOG_CHAR(log, S[i]); i++; } \
    }
*/
void __output_properties(const char *key, const char *value, const void *obj) {
    
    message_stream* stream = ((file_pair *) obj)->stream;
    trax_logging* log = ((file_pair *) obj)->log;

    OUTPUT_STRING("\"");
    OUTPUT_STRING(key);
    OUTPUT_STRING("=");
    OUTPUT_ESCAPED(value);
    OUTPUT_STRING("\" ");

}

void write_message(message_stream* stream, trax_logging* log, int type, const string_list* arguments, trax_properties* properties) {

    int i;

    assert(type >= TRAX_HELLO && type <= TRAX_STATE);

    VALIDATE_MESSAGE_STREAM(stream);

    switch (type) {
        case TRAX_HELLO:
            OUTPUT_STRING(TRAX_PREFIX);
            OUTPUT_STRING("hello");
            break;
        case TRAX_INITIALIZE:
            OUTPUT_STRING(TRAX_PREFIX);
            OUTPUT_STRING("initialize");
            break;            
        case TRAX_FRAME:
            OUTPUT_STRING(TRAX_PREFIX);
            OUTPUT_STRING("frame");
            break;
        case TRAX_STATE:
            OUTPUT_STRING(TRAX_PREFIX);
            OUTPUT_STRING("state");
            break;
        case TRAX_QUIT:
            OUTPUT_STRING(TRAX_PREFIX);
            OUTPUT_STRING("quit");
            break;
        default: {
            return;
        }
    }

    OUTPUT_STRING(" ");

    for (i = 0; i < list_size(arguments); i++) {
        char* arg = arguments->buffer[i];
        OUTPUT_STRING("\"");
        OUTPUT_ESCAPED(arg);
        OUTPUT_STRING("\" ");
    }

    {

        file_pair pair;
        pair.stream = stream;
        pair.log = log;

        trax_properties_enumerate(properties, __output_properties, &pair);

        OUTPUT_STRING("\n");

        LOG_BUFFER(log, NULL, 0); // Flush the log stream

    }


}

