
#ifndef __STRING_BUFFER_H
#define __STRING_BUFFER_H

#include <string.h>
#include <stdlib.h>

typedef struct string_buffer {
	char* buffer;
	int position;
	int size;
} string_buffer;

typedef struct string_list {
	char** buffer;
	int position;
	int size;
} string_list;

#define BUFFER_CREATE(B, L) {(B).size = L; (B).buffer = (char*) malloc(sizeof(char) * (B).size); (B).position = 0;}

#define BUFFER_RESET(B) {(B).position = 0;}

#define BUFFER_DESTROY(B) { if ((B).buffer) { free((B).buffer); (B).buffer = NULL; } }

#define BUFFER_EXTRACT(B, S) { S = (char*) malloc(sizeof(char) * ((B).position + 1)); \
		 memcpy(S, (B).buffer, (B).position); \
		 S[(B).position] = '\0'; \
	}

#define BUFFER_SIZE(B) (B).position

#define BUFFER_PUSH(B, C) { \
		int required = 1; \
		if (required > (B).size - (B).position) { \
			(B).size = (B).position + 512; \
			(B).buffer = (char*) realloc((B).buffer, sizeof(char) * (B).size); \
		} \
        (B).buffer[(B).position] = C; \
		(B).position += required; \
  }

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 

#define BUFFER_APPEND(B, ...) { \
		int required = _scprintf(__VA_ARGS__); \
		if (required > (B).size - (B).position) { \
			(B).size = (B).position + required + 1; \
			(B).buffer = (char*) realloc((B).buffer, sizeof(char) * (B).size); \
		} \
		required = _snprintf_s(&((B).buffer[(B).position]), (B).size - (B).position, _TRUNCATE, __VA_ARGS__); \
		(B).position += required; \
  }

#else

#define BUFFER_APPEND(B, ...) { \
		int required = snprintf(&(B.buffer[(B).position]), (B).size - (B).position, __VA_ARGS__); \
		if (required > (B).size - (B).position) { \
			(B).size = (B).position + required + 1;  \
			(B).buffer = (char*) realloc((B).buffer, sizeof(char) * (B).size); \
			required = snprintf(&((B).buffer[(B).position]), (B).size - (B).position, __VA_ARGS__); \
		} \
		(B).position += required; \
  }

#endif


#define LIST_CREATE(B, L) {(B).size = L; (B).buffer = (char**) malloc(sizeof(char*) * (B).size); memset((B).buffer, 0, sizeof(char*) * (B).size); (B).position = 0;}

#define LIST_RESET(B) { int i; for (i = 0; i < (B).position; i++) { if ((B).buffer[i]) free((B).buffer[i]); (B).buffer[i] = NULL; } (B).position = 0;}

#define LIST_DESTROY(B) { int i; for (i = 0; i < (B).position; i++) { if ((B).buffer[i]) free((B).buffer[i]); (B).buffer[i] = NULL; } if ((B).buffer) { free((B).buffer); (B).buffer = NULL; } }

#define LIST_GET(B, I, S) { if (I < 0 || I >= (B).position) { S = NULL; } else { \
        if (!(B).buffer[I]) { S = NULL; } else { \
        int length = strlen((B).buffer[I]); \
        S = (char*) malloc(sizeof(char) * (length + 1)); \
		 memcpy(S, (B).buffer[I], length+1); \
        } } \
	}

#define LIST_SIZE(B) (B).position

#define LIST_APPEND(B, S) { \
		int required = 1; \
        int length = strlen(S); \
		if (required > (B).size - (B).position) { \
			(B).size = (B).position + 16; \
			(B).buffer = (char**) realloc((B).buffer, sizeof(char*) * (B).size); \
		} \
        (B).buffer[(B).position] = (char*) malloc(sizeof(char) * (length + 1)); \
        memcpy((B).buffer[(B).position], S, length+1); \
		(B).position += required; \
  }


#endif
