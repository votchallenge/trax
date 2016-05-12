
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "region.h"
#include "buffer.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 
#ifndef isnan
    #define isnan(x) _isnan(x)
#endif
#ifndef isinf
    #define isinf(x) (!_finite(x))
#endif
#endif

#if defined (_MSC_VER)
#define INFINITY (DBL_MAX+DBL_MAX)
#define NAN (INFINITY-INFINITY)
#endif

#define PRINT_BOUNDS(B) printf("[left: %.2f, top: %.2f, right: %.2f, bottom: %.2f]\n", B.left, B.top, B.right, B.bottom)

const region_bounds region_no_bounds = { -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX };

int __is_valid_sequence(float* sequence, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (isnan(sequence[i])) return 0;
    }

    return 1;
}


region_container* __create_region(region_type type) {

    region_container* reg = (region_container*) malloc(sizeof(region_container));

    reg->type = type;

    return reg;

}

int _parse_sequence(char* buffer, float** data) {

    int i;

    float* numbers = (float*) malloc(sizeof(float) * (strlen(buffer) / 2));

	char* pch = strtok(buffer, ",");
    for (i = 0; ; i++) {
        
        if (pch){
            #if defined (_MSC_VER)
            if(tolower(pch[0]) == 'n' && tolower(pch[1]) == 'a' && tolower(pch[2]) == 'n'){    
               numbers[i] = NAN;
            }else{
               numbers[i] = (float) atof(pch);
            }
            #else
            numbers[i] = (float) atof(pch);
            #endif 
        }else 
            break;

		pch = strtok(NULL, ",");
    }

	if (i > 0) {
		int j;
		*data = (float*) malloc(sizeof(float) * i);
		for (j = 0; j < i; j++) { (*data)[j] = numbers[j]; } 
		free(numbers);
	} else {
		*data = NULL;
		free(numbers);
	}

    return i;
}

int region_parse(char* buffer, region_container** region) {

    float* data = NULL;
    int num;
    
	char* tmp = buffer;

	(*region) = NULL;

	if (buffer[0] == 'M') {
		/* TODO: mask */
		return 0;
	}

	num = _parse_sequence(buffer, &data);
    
    // If at least one of the elements is NaN, then the region cannot be parsed
    // We return special region with a default code.
    if (!__is_valid_sequence(data, num) || num == 0) {
        // Preserve legacy support: if four values are given and the fourth one is a number
        // then this number is taken as a code.
        if (num == 4 && !isnan(data[3])) {
            (*region) = region_create_special(-(int) data[3]);
        } else {
            (*region) = region_create_special(TRAX_DEFAULT_CODE);
        }
        free(data);
		return 1; 
    }

    if (num == 1) {
		(*region) = (region_container*) malloc(sizeof(region_container));
		(*region)->type = SPECIAL;
		(*region)->data.special = (int) data[0];
		free(data);
		return 1;

	} 
    if (num == 4) {

		(*region) = (region_container*) malloc(sizeof(region_container));
		(*region)->type = RECTANGLE;

	    (*region)->data.rectangle.x = data[0];
	    (*region)->data.rectangle.y = data[1];
	    (*region)->data.rectangle.width = data[2];
	    (*region)->data.rectangle.height = data[3];

		free(data);
		return 1;

    }
    if (num >= 6 && num % 2 == 0) {

		int j;

		(*region) = (region_container*) malloc(sizeof(region_container));
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

	if (data) free(data);

    return 0;
}

char* region_string(region_container* region) {

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

void region_print(FILE* out, region_container* region) {

	char* buffer = region_string(region);

	if (buffer) {
		fputs(buffer, out);
		free(buffer);
	}

}

region_container* region_convert(const region_container* region, region_type type) {

    region_container* reg = NULL;

	switch (type) {

		case RECTANGLE: {

			reg = (region_container*) malloc(sizeof(region_container));
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
				}
		        case SPECIAL: {
                    free(reg); reg = NULL;
                    break;
                }
				default: {
					free(reg); reg = NULL;
					break;
				}
			}
			break;
		}

		case POLYGON: {

			reg = (region_container*) malloc(sizeof(region_container));
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
				}
		        case SPECIAL: {
                    free(reg); reg = NULL;
                    break;
                }
				default: {
					free(reg); reg = NULL;
					break;
				}
			}
			break;

		case SPECIAL: {
            if (region->type == SPECIAL)
                // If source is also code then just copy the value
                reg = region_create_special(region->data.special);
            else
                // All types are converted to default region
                reg = region_create_special(TRAX_DEFAULT_CODE);
            break;
        }

		default:
			break;

		}

	}

    return reg;

}

void region_release(region_container** region) {

    switch ((*region)->type) {
        case RECTANGLE:
            break;
        case POLYGON:
            free((*region)->data.polygon.x);
            free((*region)->data.polygon.y);
            (*region)->data.polygon.count = 0;
            break;
		case SPECIAL: {
            break;
        }
    }

    free(*region);

    *region = NULL;

}

region_container* region_create_special(int code) {

    region_container* reg = __create_region(SPECIAL);

    reg->data.special = code;

    return reg;

}

region_container* region_create_rectangle(float x, float y, float width, float height) {

    region_container* reg = __create_region(RECTANGLE);

    reg->data.rectangle.width = width;
    reg->data.rectangle.height = height;
    reg->data.rectangle.x = x;
    reg->data.rectangle.y = y;

    return reg;

}

region_container* region_create_polygon(int count) {

	assert(count > 0);

	{

		region_container* reg = __create_region(POLYGON);

		reg->data.polygon.count = count;
		reg->data.polygon.x = (float *) malloc(sizeof(float) * count);
		reg->data.polygon.y = (float *) malloc(sizeof(float) * count);

		return reg;

	}
}

#define MAX_MASK 4000

void free_polygon(region_polygon* polygon) {

	free(polygon->x);
	free(polygon->y);

	polygon->x = NULL;
	polygon->y = NULL;

	polygon->count = 0;

}

region_polygon* allocate_polygon(int count) {

	region_polygon* polygon = (region_polygon*) malloc(sizeof(region_polygon));

	polygon->count = count;

	polygon->x = (float*) malloc(sizeof(float) * count);
	polygon->y = (float*) malloc(sizeof(float) * count);

	memset(polygon->x, 0, sizeof(float) * count);
	memset(polygon->y, 0, sizeof(float) * count);

	return polygon;
}

region_polygon* clone_polygon(region_polygon* polygon) {

	region_polygon* clone = allocate_polygon(polygon->count);

	memcpy(clone->x, polygon->x, sizeof(float) * polygon->count);
	memcpy(clone->y, polygon->y, sizeof(float) * polygon->count);

	return clone;
}

region_polygon* offset_polygon(region_polygon* polygon, float x, float y) {

	int i;
	region_polygon* clone = clone_polygon(polygon);

	for (i = 0; i < clone->count; i++) {
		clone->x[i] += x;
		clone->y[i] += y;
	}

	return clone;
}

void print_polygon(region_polygon* polygon) {

	int i;
	printf("%d:", polygon->count);

	for (i = 0; i < polygon->count; i++) {
		printf(" (%f, %f)", polygon->x[i], polygon->y[i]);
	}

	printf("\n");

}

region_bounds compute_bounds(region_polygon* polygon) {

	int i;
	region_bounds bounds;
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

region_bounds bounds_intersection(region_bounds a, region_bounds b) {

    region_bounds result;

    result.top = MAX(a.top, b.top);
    result.bottom = MIN(a.bottom, b.bottom);
    result.left = MAX(a.left, b.left);
    result.right = MIN(a.right, b.right);

    return result;

}

region_bounds bounds_union(region_bounds a, region_bounds b) {

    region_bounds result;

    result.top = MIN(a.top, b.top);
    result.bottom = MAX(a.bottom, b.bottom);
    result.left = MIN(a.left, b.left);
    result.right = MAX(a.right, b.right);

    return result;

}

float bounds_overlap(region_bounds a, region_bounds b) {

    region_bounds rintersection = bounds_intersection(a, b);
    float intersection = (rintersection.right - rintersection.left) * (rintersection.bottom - rintersection.top);

    return intersection / (((a.right - a.left) * (a.bottom - a.top)) + ((b.right - b.left) * (b.bottom - b.top)) - intersection);

}

region_bounds region_create_bounds(float left, float top, float right, float bottom) {
    
    region_bounds result;

    result.top = top;
    result.bottom = bottom;
    result.left = left;
    result.right = right;

    return result;
}

int rasterize_polygon(region_polygon* polygon, char* mask, int width, int height) {

	int nodes, pixelY, i, j, swap;
    int sum = 0;

	int* nodeX = (int*) malloc(sizeof(int) * polygon->count);

	if (mask) memset(mask, 0, width * height * sizeof(char));

	/*  Loop through the rows of the image. */
	for (pixelY = 0; pixelY < height; pixelY++) {

		/*  Build a list of nodes. */
		nodes = 0;
		j = polygon->count - 1;

		for (i = 0; i < polygon->count; i++) {
			if (((polygon->y[i] <= (double) pixelY) && (polygon->y[j] > (double) pixelY)) ||
					 ((polygon->y[j] <= (double) pixelY) && (polygon->y[i] > (double) pixelY))) {
                double r = (polygon->y[j] - polygon->y[i]) * (polygon->x[j] - polygon->x[i]);
                if (r)
				    nodeX[nodes++] = (int) (polygon->x[i] + (pixelY - polygon->y[i]) / r); 
                else
                    nodeX[nodes++] = (int) (polygon->x[i]); 
			}
			j = i; 
		}

		/* Sort the nodes, via a simple “Bubble” sort. */
		i = 0;
		while (i < nodes-1) {
			if (nodeX[i] > nodeX[i+1]) {
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
			if (nodeX[i+1] >= 0 ) {
				if (nodeX[i] < 0 ) nodeX[i] = 0;
				if (nodeX[i+1] > width) nodeX[i+1] = width - 1;
				for (j = nodeX[i]; j < nodeX[i+1]; j++) {
					if (mask) mask[pixelY * width + j] = 1; 
                    sum++;
                }
			}
		}
	}

	free(nodeX);

    return sum;
}

float compute_polygon_overlap(region_polygon* p1, region_polygon* p2, float *only1, float *only2, region_bounds bounds) {

    int i;
    int vol_1 = 0;
    int vol_2 = 0;
    int mask_1 = 0;
    int mask_2 = 0;
    int mask_intersect = 0;
    char* mask1 = NULL;
    char* mask2 = NULL;
	region_polygon *op1, *op2;

	region_bounds b1 = bounds_intersection(compute_bounds(p1), bounds);
	region_bounds b2 = bounds_intersection(compute_bounds(p2), bounds);

	float x = MIN(b1.left, b2.left);
	float y = MIN(b1.top, b2.top);

	int width = (int) (MAX(b1.right, b2.right) - x);	
	int height = (int) (MAX(b1.bottom, b2.bottom) - y);

    if (bounds_overlap(b1, b2) == 0) {

        if (only1 || only2) {
	        vol_1 = rasterize_polygon(p1, NULL, b1.right - b1.left + 1, b1.bottom - b1.top + 1); 
	        vol_2 = rasterize_polygon(p2, NULL, b2.right - b2.left + 1, b2.bottom - b2.top + 1); 

	        if (only1)
		        (*only1) = (float) vol_1 / (float) (vol_1 + vol_2);

	        if (only2)
		        (*only2) = (float) vol_2 / (float) (vol_1 + vol_2);
        }

        return 0;

    }

	mask1 = (char*) malloc(sizeof(char) * width * height);
	mask2 = (char*) malloc(sizeof(char) * width * height);

	op1 = offset_polygon(p1, -x, -y);
	op2 = offset_polygon(p2, -x, -y);

	rasterize_polygon(op1, mask1, width, height); 
	rasterize_polygon(op2, mask2, width, height); 

	for (i = 0; i < width * height; i++) {
        if (mask1[i]) vol_1++;
        if (mask2[i]) vol_2++;
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

region_overlap region_compute_overlap(region_container* ra, region_container* rb, region_bounds bounds) {

    region_container* ta = ra;
    region_container* tb = rb;
    region_overlap overlap;
	overlap.overlap = 0;
	overlap.only1 = 0;
	overlap.only2 = 0;

    if (ra->type == RECTANGLE)
        ta = region_convert(ra, POLYGON);
            
    if (rb->type == RECTANGLE)
        tb = region_convert(rb, POLYGON);

    if (ta->type == POLYGON && tb->type == POLYGON) {

        region_polygon p1, p2;

        COPY_POLYGON(ta, p1);
        COPY_POLYGON(tb, p2);

        overlap.overlap = compute_polygon_overlap(&p1, &p2, &(overlap.only1), &(overlap.only2), bounds);

    }

    if (ta != ra)
        region_release(&ta);

    if (tb != rb)
        region_release(&tb);

    return overlap;

}

void region_mask(region_container* r, char* mask, int width, int height) {

    region_container* t = r;

    if (r->type == RECTANGLE)
        t = region_convert(r, POLYGON);
    
	rasterize_polygon(&(t->data.polygon), mask, width, height); 

    if (t != r)
        region_release(&t);

}
