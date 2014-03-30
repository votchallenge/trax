
#include <stdio.h>
#include <float.h>

#include "region.h"

int _parse_sequence(char* buffer, float** data) {

    int i;

    float* numbers = (float*) malloc(sizeof(float) * strlen(buffer) / 2);

    for (i = 0; ; i++) {
        char* pch = strtok(buffer, ",");
        if (pch)
            numbers[i] = atof(pch);
        else 
            break;
    }

    *data = (float*) realloc(data, sizeof(float) * (i - 1));

    return i - 1;
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

void print_region(FILE* out, trax_region* region) {

	int i;

	if (region->type = TRAX_REGION_RECTANGLE) {
		fprintf(out, "%f,%f,%f,%f", region->data.rectangle.x, region->data.rectangle.y, region->data.rectangle.width, region->data.rectangle.height);
	} else if (region->type = TRAX_REGION_POLYGON) {
		for (i = 0; i < region->data.polygon.count; i++)
			fprintf(out, i == 0 ? "%.2f,%.2f" : ",%.2f,%.2f", region->data.polygon.x[i], region->data.polygon.y[i]);
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

		}

	}

    return reg;

}

