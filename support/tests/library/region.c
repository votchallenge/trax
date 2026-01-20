#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "region.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER) 


#else

#include <unistd.h>
#define strcmpi strcasecmp

#endif

typedef struct {
    const char* input;
    const char* canonical;
} roundtrip_case;

static const roundtrip_case cases[] = {
    { "5,5", "5.0000,5.0000" },                                       /* implicit point */
    { "point:5,5", "5.0000,5.0000" },                                 /* prefixed point */
    { "rect:10,10,10,10", "10.0000,10.0000,10.0000,10.0000" },        /* rectangle */
    { "10,10,20,20,30,10", "10.0000,10.0000,20.0000,20.0000,30.0000,10.0000" }, /* polygon (3 pts) */
    { "10,10,20,10,20,20,10,20", "10.0000,10.0000,20.0000,10.0000,20.0000,20.0000,10.0000,20.0000" }, /* polygon (4 pts) */
    { "mask:10,10,30,30,5,10,5,10,5,1,20,30,2,30,2,30,30,30,2,30,5,7,23,2",
      "mask:10,10,30,30,5,10,5,10,5,1,20,30,2,30,2,30,30,30,2,30,5,7,23,2" },   /* mask RLE */
    { "special:7", "7" },                                             /* special code */
    { "nan,nan", "0" },                                               /* invalid → default special */
    { NULL, NULL }
};

static void test_roundtrips(void) {

    for (int i = 0; cases[i].input; i++) {

        const char* source = cases[i].input;
        const char* canonical = cases[i].canonical;
        char* a = NULL;
        char* b = NULL;
        region_container *r1 = NULL, *r2 = NULL;

        assert(region_parse(source, &r1));
        a = region_string(r1);
        assert(a);
        assert(strcmpi(a, canonical) == 0);

        assert(region_parse(a, &r2));
        b = region_string(r2);
        assert(b);
        assert(strcmpi(b, canonical) == 0);

        printf("%s ** %s ** %s\n", source, a, b);

        free(a);
        free(b);
        region_release(&r1);
        region_release(&r2);
    }
}

static void test_point_properties(void) {

    region_container* p = region_create_point(5.0f, 5.0f);
    region_container* r = region_convert(p, RECTANGLE);
    region_bounds b = region_compute_bounds(p);

    assert(p);
    assert(r);
    assert(b.left == 5.0f && b.right == 5.0f && b.top == 5.0f && b.bottom == 5.0f);
    assert(region_contains_point(p, 5.0f, 5.0f) == 1);
    assert(region_contains_point(p, 6.0f, 5.0f) == 0);

    assert(r->type == RECTANGLE);
    assert(r->data.rectangle.width == 0.0f);
    assert(r->data.rectangle.height == 0.0f);
    assert(r->data.rectangle.x == 5.0f && r->data.rectangle.y == 5.0f);

    region_release(&p);
    region_release(&r);
}

int main(int argc, char** argv) {
    (void) argc; (void) argv;
    test_roundtrips();
    test_point_properties();
    return 0;
}



