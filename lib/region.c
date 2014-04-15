
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "region.h"
#include "buffer.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

typedef struct Bounds {

	float top;
	float bottom;
	float left;
	float right;

} Bounds;

Region* __create_region(int type) {

    Region* reg = (Region*) malloc(sizeof(Region));

    reg->type = type;

    return reg;

}

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

int region_parse(char* buffer, Region** region) {

    float* data = NULL;
    int num;
    
	(*region) = NULL;

	if (buffer[0] == '<') {
		/* TODO: mask */
		return 0;
	}

	num = _parse_sequence(buffer, &data);

    if (num == 1) {
		(*region) = (Region*) malloc(sizeof(Region));
		(*region)->type = SPECIAL;
		(*region)->data.special = (int) data[0];

		free(data);
		return 1;

	} else if (num == 4) {

		if (isnan(data[0]) && !isnan(data[3])) {

			/* Support for old rectangle format */
			(*region) = (Region*) malloc(sizeof(Region));
			(*region)->type = SPECIAL;
			(*region)->data.special = -(int) data[3];

		} else {

			(*region) = (Region*) malloc(sizeof(Region));
			(*region)->type = RECTANGLE;

		    (*region)->data.rectangle.x = data[0];
		    (*region)->data.rectangle.y = data[1];
		    (*region)->data.rectangle.width = data[2];
		    (*region)->data.rectangle.height = data[3];

		}

		free(data);
		return 1;

    } else if (num >= 6 && num % 2 == 0) {

		int j;

		(*region) = (Region*) malloc(sizeof(Region));
		(*region)->type = POLYGON;

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

char* region_string(Region* region) {

	int i;
	char* result = NULL;
	string_buffer buffer;

	BUFFER_CREATE(buffer, 32);

	if (region->type == SPECIAL) {

		BUFFER_APPEND(buffer, "%d", region->data.special);	

	} else if (region->type == RECTANGLE) {

		BUFFER_APPEND(buffer, "%f,%f,%f,%f", 
			region->data.rectangle.x, region->data.rectangle.y, 
			region->data.rectangle.width, region->data.rectangle.height);
		
	} else if (region->type == POLYGON) {
		for (i = 0; i < region->data.polygon.count; i++)
			BUFFER_APPEND(buffer, i == 0 ? "%.2f,%.2f" : ",%.2f,%.2f", region->data.polygon.x[i], region->data.polygon.y[i]);
	}

	if (BUFFER_SIZE(buffer) > 0)
		BUFFER_EXTRACT(buffer, result);

	BUFFER_DESTROY(buffer);

	return result;
}

void region_print(FILE* out, Region* region) {

	char* buffer = region_string(region);

	if (buffer) {
		fputs(buffer, out);
		free(buffer);
	}

}

Region* region_convert(const Region* region, int type) {

    Region* reg = NULL;

	switch (type) {

		case RECTANGLE: {

			reg = (Region*) malloc(sizeof(Region));
			reg->type = type;

			switch (region->type) {
				case RECTANGLE:
				    reg->data.rectangle = region->data.rectangle;
				    break;
				case POLYGON: {

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
				default:
					free(reg); reg = NULL;
					break;
				}
			}
			break;
		}

		case POLYGON: {

			reg = (Region*) malloc(sizeof(Region));
			reg->type = type;

			switch (region->type) {
				case RECTANGLE: {

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
				case POLYGON: {

					reg->data.polygon.count = region->data.polygon.count;

					reg->data.polygon.x = (float *) malloc(sizeof(float) * region->data.polygon.count);
					reg->data.polygon.y = (float *) malloc(sizeof(float) * region->data.polygon.count);

					memcpy(reg->data.polygon.x, region->data.polygon.x, sizeof(float) * region->data.polygon.count);
					memcpy(reg->data.polygon.y, region->data.polygon.y, sizeof(float) * region->data.polygon.count);

				    break;
				default:
					free(reg); reg = NULL;
					break;
				}
			}
			break;

		default:
			break;

		}

	}

    return reg;

}

void region_release(Region** region) {

    switch ((*region)->type) {
        case RECTANGLE:
            break;
        case POLYGON:
            free((*region)->data.polygon.x);
            free((*region)->data.polygon.y);
            (*region)->data.polygon.count = 0;
            break;
    }

    free(*region);

    *region = NULL;

}

Region* region_create_special(int code) {

    Region* reg = __create_region(SPECIAL);

    reg->data.special = code;

    return reg;

}

Region* region_create_rectangle(float x, float y, float width, float height) {

    Region* reg = __create_region(RECTANGLE);

    reg->data.rectangle.width = width;
    reg->data.rectangle.height = height;
    reg->data.rectangle.x = x;
    reg->data.rectangle.y = y;

    return reg;

}

Region* region_create_polygon(int count) {

	assert(count > 0);

	{

		Region* reg = __create_region(POLYGON);

		reg->data.polygon.count = count;
		reg->data.polygon.x = (float *) malloc(sizeof(float) * count);
		reg->data.polygon.y = (float *) malloc(sizeof(float) * count);

		return reg;

	}
}

#define MAX_MASK 4000

void free_polygon(Polygon* polygon) {

	free(polygon->x);
	free(polygon->y);

	polygon->x = NULL;
	polygon->y = NULL;

	polygon->count = 0;

}

Polygon* allocate_polygon(int count) {

	Polygon* polygon = (Polygon*) malloc(sizeof(Polygon));

	polygon->count = count;

	polygon->x = (float*) malloc(sizeof(float) * count);
	polygon->y = (float*) malloc(sizeof(float) * count);

	memset(polygon->x, 0, sizeof(float) * count);
	memset(polygon->y, 0, sizeof(float) * count);

	return polygon;
}

Polygon* clone_polygon(Polygon* polygon) {

	Polygon* clone = allocate_polygon(polygon->count);

	memcpy(clone->x, polygon->x, sizeof(float) * polygon->count);
	memcpy(clone->y, polygon->y, sizeof(float) * polygon->count);

	return clone;
}

Polygon* offset_polygon(Polygon* polygon, float x, float y) {

	int i;
	Polygon* clone = clone_polygon(polygon);

	for (i = 0; i < clone->count; i++) {
		clone->x[i] += x;
		clone->y[i] += y;
	}

	return clone;
}

void print_polygon(Polygon* polygon) {

	int i;
	printf("%d:", polygon->count);

	for (i = 0; i < polygon->count; i++) {
		printf(" (%f, %f)", polygon->x[i], polygon->y[i]);
	}

	printf("\n");

}

Bounds compute_bounds(Polygon* polygon) {

	int i;
	Bounds bounds;
	bounds.top = MAX_MASK;
	bounds.bottom = -MAX_MASK;
	bounds.left = MAX_MASK;
	bounds.right = -MAX_MASK;

	for (i = 0; i < polygon->count; i++) {
		bounds.top = MIN(bounds.top, polygon->y[i]);
		bounds.bottom = MAX(bounds.bottom, polygon->y[i]);
		bounds.left = MIN(bounds.left, polygon->x[i]);
		bounds.right = MAX(bounds.right, polygon->x[i]);
	}

	return bounds;

}

void rasterize_polygon(Polygon* polygon, char* mask, int width, int height) {

	int nodes, pixelY, i, j, swap;

	int* nodeX = (int*) malloc(sizeof(int) * polygon->count);

	memset(mask, 0, width * height * sizeof(char));

	/*  Loop through the rows of the image. */
	for (pixelY = 0; pixelY < height; pixelY++) {

		/*  Build a list of nodes. */
		nodes = 0;
		j = polygon->count - 1;

		for (i = 0; i < polygon->count; i++) {
			if (polygon->y[i] < (double) pixelY && polygon->y[j] >= (double) pixelY ||
					 polygon->y[j] < (double) pixelY && polygon->y[i] >= (double) pixelY) {
				nodeX[nodes++] = (int) (polygon->x[i] + (pixelY - polygon->y[i]) /
					 (polygon->y[j] - polygon->y[i]) * (polygon->x[j] - polygon->x[i])); 
			}
			j = i; 
		}

		/* Sort the nodes, via a simple “Bubble” sort. */
		i = 0;
		while (i < nodes-1) {
			if (nodeX[i]>nodeX[i+1]) {
				swap = nodeX[i];
				nodeX[i] = nodeX[i+1];
				nodeX[i+1] = swap; 
				if (i) i--; 
			} else {
				i++; 
			}
		}

		/*  Fill the pixels between node pairs. */
		for (i=0; i<nodes; i+=2) {
			if (nodeX[i] >= width) break;
			if (nodeX[i+1] > 0 ) {
				if (nodeX[i] < 0 ) nodeX[i] = 0;
				if (nodeX[i+1] > width) nodeX[i+1] = width - 1;
				for (j = nodeX[i]; j < nodeX[i+1]; j++)
					mask[pixelY * width + j] = 1; 
			}
		}
	}

	free(nodeX);

}

float compute_polygon_overlap(Polygon* p1, Polygon* p2, float *only1, float *only2) {

	int i;
    int mask_1 = 0;
	int mask_2 = 0;
	int mask_intersect = 0;

	Bounds b1 = compute_bounds(p1);
	Bounds b2 = compute_bounds(p2);

	float x = MIN(b1.left, b2.left);
	float y = MIN(b1.top, b2.top);

	int width = (int) (MAX(b1.right, b2.right) - x);	
	int height = (int) (MAX(b1.bottom, b2.bottom) - y);

	char* mask1 = (char*) malloc(sizeof(char) * width * height);
	char* mask2 = (char*) malloc(sizeof(char) * width * height);

	Polygon* op1 = offset_polygon(p1, -x, -y);
	Polygon* op2 = offset_polygon(p2, -x, -y);
/*
print_polygon(p1);print_polygon(p2);
print_polygon(op1);print_polygon(op2);
*/
	rasterize_polygon(op1, mask1, width, height); 
	rasterize_polygon(op2, mask2, width, height); 

	for (i = 0; i < width * height; i++) {
		if (mask1[i] && mask2[i]) mask_intersect++;
        else if (mask1[i]) mask_1++;
        else if (mask2[i]) mask_2++;
	}

	free_polygon(op1);
	free_polygon(op2);

	free(mask1);
	free(mask2);

	if (only1)
		(*only1) = (float) mask_1 / (float) (mask_1 + mask_2 + mask_intersect);

	if (only2)
		(*only2) = (float) mask_2 / (float) (mask_1 + mask_2 + mask_intersect);

	return (float) mask_intersect / (float) (mask_1 + mask_2 + mask_intersect);

}

#define COPY_POLYGON(TP, P) { P.count = TP->data.polygon.count; P.x = TP->data.polygon.x; P.y = TP->data.polygon.y; }

Overlap region_compute_overlap(Region* ra, Region* rb) {

    Region* ta = ra;
    Region* tb = rb;
    Overlap overlap;
	overlap.overlap = 0;
	overlap.only1 = 0;
	overlap.only2 = 0;

    if (ra->type == RECTANGLE)
        ta = region_convert(ra, POLYGON);
            
    if (rb->type == RECTANGLE)
        tb = region_convert(rb, POLYGON);

    if (ta->type == POLYGON && tb->type == POLYGON) {

        Polygon p1, p2;

        COPY_POLYGON(ta, p1);
        COPY_POLYGON(tb, p2);

        overlap.overlap = compute_polygon_overlap(&p1, &p2, &(overlap.only1), &(overlap.only2));

    }

    if (ta != ra)
        region_release(&ta);

    if (tb != rb)
        region_release(&tb);

    return overlap;

}


