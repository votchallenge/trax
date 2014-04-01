
#ifndef _OVERLAP_H_
#define _OVERLAP_H_

typedef struct Polygon {

	int count;

	float* x;
	float* y;

} Polygon;

typedef struct Bounds {

	float top;
	float bottom;
	float left;
	float right;

} Bounds;


float compute_overlap(Polygon* p1, Polygon* p2);

#endif
