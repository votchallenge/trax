

#include "message.h"

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

#define __INLINE 

#else

#ifdef _MAC_
    #include <tcpd.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <sys/select.h>
    #include <arpa/inet.h>
    #include <netinet/tcp.h>
    #define closesocket close
#endif

#define __INLINE static inline

#define strcmpi strcasecmp

static void initialize_sockets(void) {}

#endif

#define VALIDATE_MESSAGE_STREAM(S) assert((S->flags & TRAX_STREAM_FILES) || (S->flags & TRAX_STREAM_SOCKET))

int __is_valid_key(char* c, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (!(isalnum(c[i]) || c[i] == '.' || c[i] == '_')) return 0;
    }

    return 1;
}

int __parse_message_type(const char *str) {
    
    if (strcmpi(str, "hello") == 0) return TRAX_HELLO;
    if (strcmpi(str, "initialize") == 0) return TRAX_INITIALIZE;
    if (strcmpi(str, "state") == 0) return TRAX_STATUS;
    if (strcmpi(str, "status") == 0) return TRAX_STATUS;
    if (strcmpi(str, "frame") == 0) return TRAX_FRAME;
    if (strcmpi(str, "quit") == 0) return TRAX_QUIT;

    return -1;
}


message_stream* create_message_stream_file(int input, int output) {
    message_stream* stream = (message_stream*) malloc(sizeof(message_stream));

    stream->flags = TRAX_STREAM_FILES;
    stream->files.input = input;
    stream->files.output = output;
    stream->buffer_position = 0;
    stream->buffer_length = 0;

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

    return stream;

}

message_stream* create_message_stream_socket_accept(int server) {

	fd_set readfds,writefds,exceptfds;
	int asock=-1;
    int one = 1;
    message_stream* stream = NULL;

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

    select(server+1,&readfds,&writefds,&exceptfds,(struct timeval *)0);
	
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

    free(*stream);
    *stream = 0;

}

__INLINE int read_character(message_stream* stream) {
    char chr;

    if (stream->buffer_position >= stream->buffer_length) {

        if (stream->flags & TRAX_STREAM_SOCKET) {

		    stream->buffer_length = recv(stream->socket.socket, stream->buffer, TRAX_BUFFER_SIZE, 0);

        } else {

        	stream->buffer_length = read(stream->files.input, stream->buffer, TRAX_BUFFER_SIZE);
           
        }

        if (stream->buffer_length < 0) return -1; // An error has occured

		if (stream->buffer_length == 0) return -1; // The stream was closed

        stream->buffer_position = 0;
    }

    chr = stream->buffer[stream->buffer_position];

    stream->buffer_position++;

    return chr;

}

__INLINE int write_string(message_stream* stream, const char* buf, int len) {
    char chr;

    if (stream->flags & TRAX_STREAM_SOCKET) {

	    int cnt = 0;

	    while(cnt < len) {
		    int l = send(stream->socket.socket, buf+cnt, len-cnt,0);
		    if(l == -1) {
			    return -1;
		    }
		    cnt += l;
	    }

    } else {

    	write(stream->files.output, buf, len);
       
    }

	return 1;
}


int read_message(message_stream* stream, trax_logger log, string_list* arguments, trax_properties* properties) {
	
	int message_type = -1;
    int prefix_length = strlen(TRAX_PREFIX);
	int complete = FALSE;
    int state = -prefix_length;
	string_buffer key_buffer, value_buffer;

	VALIDATE_MESSAGE_STREAM(stream);

    BUFFER_CREATE(key_buffer, 512);
    BUFFER_CREATE(value_buffer, 512);
    LIST_RESET((*arguments));

    while (!complete) {

    	char chr; 
    	int val = read_character(stream);

    	//printf("%d\n", val);
    	if (val < 0) {
    		if (message_type == -1) break;
    		chr = '\n';
    		complete = TRUE;
    	} else chr = (char) val;

        if (log) log(&chr);

        switch (state) {
            case PARSE_STATE_TYPE: { // Parsing message type

                if (isalnum(chr)) {

                    BUFFER_PUSH(key_buffer, chr);

                } else if (chr == ' ') {
                    char* key_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);
                	message_type = __parse_message_type(key_tmp);
                    free(key_tmp);
                    
                    if (message_type == -1) {
                		state = PARSE_STATE_PASS;
                		message_type = -1;
                    } else {
                        state = PARSE_STATE_SPACE; 
                    }

                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);

                } else if (chr == '\n') {
                    char* key_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);
                	message_type = __parse_message_type(key_tmp);
                    free(key_tmp);

                    if (message_type == -1) {
                		state = PARSE_STATE_PASS;
                		message_type = -1;
                    } else {
                        complete = TRUE;
                    }

                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                    
                } else {
                    state = PARSE_STATE_PASS;
                    BUFFER_RESET(key_buffer);
                }

                break;
            }
            case PARSE_STATE_SPACE_EXPECT: {

                if (chr == ' ') {
                    state = PARSE_STATE_SPACE;
                } else if (chr == '\n') {
                	complete = TRUE;
                } else {
                	message_type = -1;
                	state = PARSE_STATE_PASS;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                    BUFFER_PUSH(key_buffer, chr);             	
                }

                break;

            }		            
            case PARSE_STATE_SPACE: {

              if (chr == ' ' || chr == '\r') {
                    // Do nothing
                } else if (chr == '\n') {
                	complete = TRUE;
                } else if (chr == '"') {
                    state = PARSE_STATE_QUOTED_KEY;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                } else {
                	state = PARSE_STATE_UNQUOTED_KEY;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                    BUFFER_PUSH(key_buffer, chr);              	
                }

                break;

            }	            
            case PARSE_STATE_UNQUOTED_KEY: {

                if (chr == '\\') {
                    state = PARSE_STATE_UNQUOTED_ESCAPE_KEY;
                } else if (chr == '\n') { // append arg and finalize
                    char* key_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);            
                    LIST_APPEND(*arguments, key_tmp);
                    free(key_tmp);

                    complete = TRUE;
                } else if (chr == ' ') { // append arg and move on
                    char* key_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);            
                    LIST_APPEND(*arguments, key_tmp);
                    free(key_tmp);

                    state = PARSE_STATE_SPACE;
                    BUFFER_RESET(key_buffer);
                } else if (chr == '=') { // we have a kwarg
                	if (__is_valid_key(key_buffer.buffer, key_buffer.position))
                    	state = PARSE_STATE_UNQUOTED_VALUE;
                	else {
                		state = PARSE_STATE_PASS;
                		message_type = -1;
                	}
                } else {                    
                	BUFFER_PUSH(key_buffer, chr);
                } 

                break;

            }
            case PARSE_STATE_UNQUOTED_VALUE: {

                if (chr == '\\') {
                    state = PARSE_STATE_UNQUOTED_ESCAPE_VALUE;
                } else if (chr == ' ') {
                    char *key_tmp, *value_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);
                    BUFFER_EXTRACT(value_buffer, value_tmp);                
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    state = PARSE_STATE_SPACE;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);         
                } else if (chr == '\n') {
                    char *key_tmp, *value_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);
                    BUFFER_EXTRACT(value_buffer, value_tmp);                
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    complete = TRUE;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                } else
                    BUFFER_PUSH(value_buffer, chr);

                break;

            }
            case PARSE_STATE_UNQUOTED_ESCAPE_KEY: {

                if (chr == 'n') {
                	BUFFER_PUSH(key_buffer, '\n');
                    state = PARSE_STATE_UNQUOTED_KEY;
                } else if (chr != '\n') {
                	BUFFER_PUSH(key_buffer, chr);
                    state = PARSE_STATE_UNQUOTED_KEY;
                } else {
                    state = PARSE_STATE_PASS;
                    message_type = -1;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                }
                
                break;

            }	            
            case PARSE_STATE_UNQUOTED_ESCAPE_VALUE: {

                if (chr == 'n') {
                    BUFFER_PUSH(value_buffer, '\n');
                    state = PARSE_STATE_UNQUOTED_VALUE;
                } else if (chr != '\n') {
                    BUFFER_PUSH(value_buffer, chr);
                    state = PARSE_STATE_UNQUOTED_VALUE;
                } else {
                    state = PARSE_STATE_PASS;
                    message_type = -1;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                }
                
                break;

            }

            case PARSE_STATE_QUOTED_KEY: {

                if (chr == '\\') {
                    state = PARSE_STATE_QUOTED_ESCAPE_KEY;
                } else if (chr == '"') { // append arg and move on
                    char *key_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);            
                    LIST_APPEND((*arguments), key_tmp);
                    free(key_tmp);

                	state = PARSE_STATE_SPACE_EXPECT;
                } else if (chr == '=') { // we have a kwarg
                	if (__is_valid_key(key_buffer.buffer, key_buffer.position))
                    	state = PARSE_STATE_QUOTED_VALUE;
                	else {
                		message_type = -1;
                		state = PARSE_STATE_PASS;	                		
                	}
                } else {                    
                	BUFFER_PUSH(key_buffer, chr);
                } 

                break;

            }
            case PARSE_STATE_QUOTED_VALUE: {

                if (chr == '\\') {
                    state = PARSE_STATE_QUOTED_ESCAPE_VALUE;
                } else if (chr == '"') {
                    char *key_tmp, *value_tmp;
                    BUFFER_EXTRACT(key_buffer, key_tmp);
                    BUFFER_EXTRACT(value_buffer, value_tmp);                
                    trax_properties_set(properties, key_tmp, value_tmp);
                    free(key_tmp); free(value_tmp);

                    state = PARSE_STATE_SPACE_EXPECT;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                } else
                    BUFFER_PUSH(value_buffer, chr);

                break;

            }
            case PARSE_STATE_QUOTED_ESCAPE_KEY: {

                if (chr == 'n') {
                	BUFFER_PUSH(key_buffer, '\n');
                    state = PARSE_STATE_QUOTED_KEY;
                } else if (chr != '\n') {
                	BUFFER_PUSH(key_buffer, chr);
                    state = PARSE_STATE_QUOTED_KEY;
                } else {
                    state = PARSE_STATE_PASS;
                    message_type = -1;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                }
                
                break;

            }		            
            case PARSE_STATE_QUOTED_ESCAPE_VALUE: {

                if (chr == 'n') {
                    BUFFER_PUSH(value_buffer, '\n');
                    state = PARSE_STATE_QUOTED_VALUE;
                } else if (chr != '\n') {
                    BUFFER_PUSH(value_buffer, chr);
                    state = PARSE_STATE_QUOTED_VALUE;
                } else {
                    state = PARSE_STATE_PASS;
                    message_type = -1;
                    BUFFER_RESET(key_buffer);
                    BUFFER_RESET(value_buffer);
                }
                
                break;

            }	            
            
            case PARSE_STATE_PASS: {

                if (chr == '\n') 
                    state = -prefix_length;

                break;
            }
            default: { // Parsing prefix

                if (state < 0) {
                    if (chr == TRAX_PREFIX[prefix_length + state])
                    	// When done, go to type parsing
                        state++; 
                    else 
                    	// Not a message
                        state = chr == '\n' ? -prefix_length : PARSE_STATE_PASS; 
                }

                break;
            }
        }

    }

    BUFFER_DESTROY(key_buffer);
    BUFFER_DESTROY(value_buffer);

    if (log) log(NULL); // Flush the log stream

    return message_type;
    
}

#define OUTPUT_STRING(S) { int len = strlen(S); write_string(stream, S, len); if (log) log(S); }
#define OUTPUT_ESCAPED(S) { int i = 0; while (1) { \
    if (!S[i]) break; \
    if (S[i] == '"' || S[i] == '\\') { write_string(stream, "\\", 1); if (log) log("\\"); } \
     write_string(stream, &(S[i]), 1); if (log) log(&(S[i])); i++; } \
    }

typedef struct file_pair {
    message_stream* stream;
    trax_logger log;
} file_pair;

void __output_properties(const char *key, const char *value, const void *obj) {
    
    message_stream* stream = ((file_pair *) obj)->stream;
    trax_logger log = ((file_pair *) obj)->log;

    OUTPUT_STRING("\"");
    OUTPUT_STRING(key);
    OUTPUT_STRING("=");
    OUTPUT_ESCAPED(value);
    OUTPUT_STRING("\" ");

}

void write_message(message_stream* stream, trax_logger log, int type, const string_list arguments, trax_properties* properties) {

    int i;

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
        default:
            return;
    }

    OUTPUT_STRING(" ");

    for (i = 0; i < LIST_SIZE(arguments); i++) {
        char* arg = arguments.buffer[i];
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

        if (log) log(NULL); // Flush the log stream

	}
}

