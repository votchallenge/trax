/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef _REGION_H_
#define _REGION_H_

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum RegionType {SPECIAL, RECTANGLE, POLYGON};

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

Overlap region_compute_overlap(Region* ra, Region* rb);

int region_parse(char* buffer, Region** region);

char* region_string(Region* region);

void region_print(FILE* out, Region* region);

Region* region_convert(const Region* region, int type);

void region_release(Region** region);

Region* region_create_special(int code);

Region* region_create_rectangle(float x, float y, float width, float height);

Region* region_create_polygon(int count);

#ifdef __cplusplus
}
#endif

#endif
