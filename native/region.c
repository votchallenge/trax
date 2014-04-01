
#include <stdio.h>
#include <float.h>

#include "region.h"

int _parse_sequence(char* buffer, float** data) {

    int i;

    float* numbers = (float*) malloc(sizeof(float) * (strlen(buffer) / 2));

	char* pch = strtok(buffer, ",");
    for (i = 0; ; i++) {
        
        if (pch)
            numbers[i] = atof(pch);
        else 
            break;

		pch = strtok(NULL, ",");
    }

    *data = (float*) realloc(numbers, sizeof(float) * i);

    return i;
}

int parse_region(char* buffer, trax_region** region) {

    float* data = NULL;
    int num;
    
	(*region) = NULL;

	if (buffer[0] == '<') {
		// TODO: mask
		return 0;
	}

	num = _parse_sequence(buffer, &data);

    if (num == 4) {
		(*region) = (trax_region*) malloc(sizeof(trax_region));
		(*region)->type = TRAX_REGION_RECTANGLE;

        (*region)->data.rectangle.x = data[0];
        (*region)->data.rectangle.y = data[1];
        (*region)->data.rectangle.width = data[2];
        (*region)->data.rectangle.height = data[3];

		free(data);
		return 1;

    } else if (num >= 6 && num % 2 == 0) {

		int j;

		(*region) = (trax_region*) malloc(sizeof(trax_region));
		(*region)->type = TRAX_REGION_POLYGON;

        (*region)->data.polygon.count = num / 2;
	    (*region)->data.polygon.x = (float*) malloc(sizeof(float) * (*region)->data.polygon.count);
	    (*region)->data.polygon.y = (float*) malloc(sizeof(float) * (*region)->data.polygon.count);

        for (j = 0; j < (*region)->data.polygon.count; j++) {
            (*region)->data.polygon.x[j] = data[j * 2];
            (*region)->data.polygon.y[j] = data[j * 2 + 1];
        }

		free(data);
		return 1;

    }

    free(data);

    return 0;
}

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
			B.size = B.position + required;  \
			B.buffer = (char*) realloc(B.buffer, sizeof(char) * B.size); \
			required = snprintf(&(B.buffer[B.position]), B.size - B.position, __VA_ARGS__); \
		} \
		B.position += required - 1; \
  }

#define BUFFER_EXTRACT(B, S) { S = (char*) malloc(sizeof(char) * (B.position + 1)); \
		 memcpy(S, B.buffer, B.position); \
		 S[B.position] = '\0'; \
	}

#define BUFFER_SIZE(B) B.position

char* string_region(trax_region* region) {

	int i;
	char* result = NULL;
	string_buffer buffer;

	BUFFER_CREATE(buffer, 10);

	if (region->type == TRAX_REGION_RECTANGLE) {

		BUFFER_APPEND(buffer, "%f,%f,%f,%f", 
			region->data.rectangle.x, region->data.rectangle.y, 
			region->data.rectangle.width, region->data.rectangle.height);
		
	} else if (region->type == TRAX_REGION_POLYGON) {
		for (i = 0; i < region->data.polygon.count; i++)
			BUFFER_APPEND(buffer, i == 0 ? "%.2f,%.2f" : ",%.2f,%.2f", region->data.polygon.x[i], region->data.polygon.y[i]);
	}

	if (BUFFER_SIZE(buffer) > 0)
		BUFFER_EXTRACT(buffer, result);

	BUFFER_DESTROY(buffer);

	return result;
}

void print_region(FILE* out, trax_region* region) {

	char* buffer = string_region(region);

	if (buffer) {
		fputs(buffer, out);
		free(buffer);
	}

}

trax_region* convert_region(const trax_region* region, int type) {

    trax_region* reg = NULL;

	switch (type) {

		case TRAX_REGION_RECTANGLE: {

			reg = (trax_region*) malloc(sizeof(trax_region));
			reg->type = type;

			switch (region->type) {
				case TRAX_REGION_RECTANGLE:
				    reg->data.rectangle = region->data.rectangle;
				    break;
				case TRAX_REGION_POLYGON: {

					float top = FLT_MAX;
					float bottom = FLT_MIN;
					float left = FLT_MAX;
					float right = FLT_MIN;
				    int i;

					for (i = 0; i < region->data.polygon.count; i++) {
						top = MIN(top, region->data.polygon.y[i]);
						bottom = MAX(bottom, region->data.polygon.y[i]);
						left = MIN(left, region->data.polygon.x[i]);
						right = MAX(right, region->data.polygon.x[i]);
					}

				    reg->data.rectangle.x = left;
				    reg->data.rectangle.y = top;
				    reg->data.rectangle.width = right - left;
				    reg->data.rectangle.height = bottom - top;
				    break;
				}
			}
			break;
		}

		case TRAX_REGION_POLYGON: {

			reg = (trax_region*) malloc(sizeof(trax_region));
			reg->type = type;

			switch (region->type) {
				case TRAX_REGION_RECTANGLE: {

					reg->data.polygon.count = 4;

					reg->data.polygon.x = (float *) malloc(sizeof(float) * reg->data.polygon.count);
					reg->data.polygon.y = (float *) malloc(sizeof(float) * reg->data.polygon.count);

					reg->data.polygon.x[0] = region->data.rectangle.x;
					reg->data.polygon.x[1] = region->data.rectangle.x + region->data.rectangle.width;
					reg->data.polygon.x[2] = region->data.rectangle.x + region->data.rectangle.width;
					reg->data.polygon.x[3] = region->data.rectangle.x;

					reg->data.polygon.y[0] = region->data.rectangle.y;
					reg->data.polygon.y[1] = region->data.rectangle.y;
					reg->data.polygon.y[2] = region->data.rectangle.y + region->data.rectangle.height;
					reg->data.polygon.y[3] = region->data.rectangle.y + region->data.rectangle.height;

				    break;
				}
				case TRAX_REGION_POLYGON: {

					reg->data.polygon.count = region->data.polygon.count;

					reg->data.polygon.x = (float *) malloc(sizeof(float) * region->data.polygon.count);
					reg->data.polygon.y = (float *) malloc(sizeof(float) * region->data.polygon.count);

					memcpy(reg->data.polygon.x, region->data.polygon.x, sizeof(float) * region->data.polygon.count);
					memcpy(reg->data.polygon.y, region->data.polygon.y, sizeof(float) * region->data.polygon.count);

				    break;
				}
			}
			break;
		}

	}

    return reg;

}

