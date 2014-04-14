
#ifndef __STRING_BUFFER_H
#define __STRING_BUFFER_H

typedef struct string_buffer {
	char* buffer;
	int position;
	int size;
} string_buffer;

#define BUFFER_CREATE(B, L) {B.size = L; B.buffer = (char*) malloc(sizeof(char) * B.size); B.position = 0;}

#define BUFFER_DESTROY(B) { if (B.buffer) { free(B.buffer); B.buffer = NULL; } }

#define BUFFER_APPEND(B, ...) { \
		int required = snprintf(&(B.buffer[B.position]), B.size - B.position, __VA_ARGS__); \
		if (required > B.size - B.position) { \
			B.size = B.position + required + 1;  \
			B.buffer = (char*) realloc(B.buffer, sizeof(char) * B.size); \
			required = snprintf(&(B.buffer[B.position]), B.size - B.position, __VA_ARGS__); \
		} \
		B.position += required; \
  }

#define BUFFER_EXTRACT(B, S) { S = (char*) malloc(sizeof(char) * (B.position + 1)); \
		 memcpy(S, B.buffer, B.position); \
		 S[B.position] = '\0'; \
	}

#define BUFFER_SIZE(B) B.position

#endif
