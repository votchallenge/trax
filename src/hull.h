/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <stdbool.h>
#include <stdlib.h>

typedef struct hull_point {
    int x, y;
} hull_point;
 
typedef struct hull_node {
    hull_point data;
    struct hull_node *next;
} hull_node;

hull_node* convex_hull(int len, hull_point p[]);

void free_node(hull_node *ptr);