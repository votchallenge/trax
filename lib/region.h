/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef _REGION_H_
#define _REGION_H_

#ifdef WIN32
#define __EXPORT __declspec(dllexport) 
#else
#define __EXPORT 
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RegionType {SPECIAL, RECTANGLE, POLYGON} RegionType;

typedef struct Polygon {

	int count;

	float* x;
	float* y;

} Polygon;

typedef struct Rectangle {

    float x;
    float y;
    float width;
    float height;

} Rectangle;

typedef struct Region {
    enum RegionType type;
    union {
        Rectangle rectangle;
        Polygon polygon;
        int special;
    } data;
} Region;

typedef struct Overlap {

	float overlap;    
    float only1;
    float only2;

} Overlap;

__EXPORT Overlap region_compute_overlap(Region* ra, Region* rb);

__EXPORT int region_parse(char* buffer, Region** region);

__EXPORT char* region_string(Region* region);

__EXPORT void region_print(FILE* out, Region* region);

__EXPORT Region* region_convert(const Region* region, RegionType type);

__EXPORT void region_release(Region** region);

__EXPORT Region* region_create_special(int code);

__EXPORT Region* region_create_rectangle(float x, float y, float width, float height);

__EXPORT Region* region_create_polygon(int count);

__EXPORT void region_mask(Region* r, char* mask, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
