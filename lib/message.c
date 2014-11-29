

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

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 


#else

#define strcmpi strcasecmp

#endif

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

int read_message(FILE* input, FILE* log, string_list* arguments, trax_properties* properties) {
		
	int message_type = -1;
	
	string_buffer key_buffer, value_buffer;

    BUFFER_CREATE(key_buffer, 512);
    BUFFER_CREATE(value_buffer, 512);
    LIST_RESET((*arguments));

    int prefix_length = strlen(TRAX_PREFIX);
	int complete = FALSE;
    int state = -prefix_length;

    while (!complete) {
    	
    	int val = fgetc(input);
    	char chr; 
    	//printf("%d\n", val);
    	if (val == EOF) {
    		if (message_type == -1) break;
    		chr = '\n';
    		complete = TRUE;
    	} else chr = (char) val;

        if (log) fputc(val, log);

        switch (state) {
            case PARSE_STATE_TYPE: { // Parsing message type

                if (isalnum(val)) {

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
                	BUFFER_PUSH(value_buffer, '\n');
                    state = PARSE_STATE_QUOTED_KEY;
                } else if (chr != '\n') {
                	BUFFER_PUSH(value_buffer, chr);
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

    if (log) fflush(log);

    return message_type;
    
}

#define OUTPUT_STRING(S) { fputs(S, output); if (log) fputs(S, log); }
#define OUTPUT_ESCAPED(S) { int i = 0; while (1) { \
    if (!S[i]) break; \
    if (S[i] == '"') { fputc('\\', output); if (log) fputc('\\', log); } \
     fputc(S[i], output); if (log) fputc(S[i], log); i++; } \
    }

typedef struct file_pair {
    FILE* output;
    FILE* log;
} file_pair;

void __output_properties(const char *key, const char *value, const void *obj) {
    
    FILE* output = ((file_pair *) obj)->output;
    FILE* log = ((file_pair *) obj)->log;

    OUTPUT_STRING("\"");
    OUTPUT_STRING(key);
    OUTPUT_STRING("=");
    OUTPUT_ESCAPED(value);
    OUTPUT_STRING("\" ");

}

void write_message(FILE* output, FILE* log, int type, const string_list arguments, trax_properties* properties) {

    int i;

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

    file_pair pair;
    pair.output = output;
    pair.log = log;

    trax_properties_enumerate(properties, __output_properties, &pair);

    OUTPUT_STRING("\n");

    fflush(output);

    if (log) fflush(log);

}

