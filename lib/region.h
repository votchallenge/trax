/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef _REGION_H_
#define _REGION_H_

#ifndef __TRAX_EXPORT
#if defined(_MSC_VER)
#if defined(_TRAX_BUILDING)
    #define __TRAX_EXPORT __declspec(dllexport)
#else
	#define __TRAX_EXPORT 
#endif
#elif defined(_GCC)
    #define __TRAX_EXPORT __attribute__((visibility("default")))
#else
    #define __TRAX_EXPORT
#endif
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define TRAX_DEFAULT_CODE 0

#ifdef __cplusplus
extern "C" {
#endif

typedef enum region_type {SPECIAL, RECTANGLE, POLYGON} region_type;

typedef struct region_bounds {

	float top;
	float bottom;
	float left;
	float right;

} region_bounds;

typedef struct region_polygon {

	int count;

	float* x;
	float* y;

} region_polygon;

typedef struct region_rectangle {

    float x;
    float y;
    float width;
    float height;

} region_rectangle;

typedef struct region_container {
    enum region_type type;
    union {
        region_rectangle rectangle;
        region_polygon polygon;
        int special;
    } data;
} region_container;

typedef struct region_overlap {

	float overlap;    
    float only1;
    float only2;

} region_overlap;

extern const region_bounds region_no_bounds; 

__TRAX_EXPORT region_overlap region_compute_overlap(region_container* ra, region_container* rb, region_bounds bounds);

__TRAX_EXPORT region_bounds region_create_bounds(float left, float top, float right, float bottom);

__TRAX_EXPORT int region_parse(char* buffer, region_container** region);

__TRAX_EXPORT char* region_string(region_container* region);

__TRAX_EXPORT void region_print(FILE* out, region_container* region);

__TRAX_EXPORT region_container* region_convert(const region_container* region, region_type type);

__TRAX_EXPORT void region_release(region_container** region);

__TRAX_EXPORT region_container* region_create_special(int code);

__TRAX_EXPORT region_container* region_create_rectangle(float x, float y, float width, float height);

__TRAX_EXPORT region_container* region_create_polygon(int count);

__TRAX_EXPORT void region_mask(region_container* r, char* mask, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
