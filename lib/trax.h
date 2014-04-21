/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef TRAX_H
#define TRAX_H

#include <stdio.h>

#if defined(_MSC_VER)
    #define __EXPORT __declspec(dllexport)
    #define __IMPORT __declspec(dllimport)
#elif defined(_GCC)
    #define __EXPORT __attribute__((visibility("default")))
    #define __IMPORT
#else
    #define __EXPORT
    #define __IMPORT
#endif

#define TRAX_VERSION 1

#define TRAX_ERROR 0
#define TRAX_INITIALIZE 1
#define TRAX_FRAME 2
#define TRAX_QUIT 3
#define TRAX_STATUS 4

#define TRAX_IMAGE_PATH 0
#define TRAX_IMAGE_URL 1 // Not implemented yet!
#define TRAX_IMAGE_DATA 2 // Not implemented yet!

#define TRAX_REGION_RECTANGLE 0
#define TRAX_REGION_POLYGON 1
#define TRAX_REGION_MASK 2 // Not implemented yet!
#define TRAX_REGION_SPECIAL -1

#define TRAX_FLAG_VALID 1
#define TRAX_FLAG_SERVER 2
#define TRAX_FLAG_TERMINATED 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct trax_image {
    short type;
    int width;
    int height;
    char* data;
} trax_image;

typedef void trax_region;

typedef struct trax_configuration {
        int format_region;
        int format_image;
} trax_configuration;

typedef struct trax_handle {
    int flags;
    int version;
    FILE* log;
    trax_configuration config;
    FILE* input;
    FILE* output;
} trax_handle;

typedef struct trax_properties trax_properties;

typedef void(*trax_enumerator)(const char *key, const char *value, const void *obj);

/**
 *
**/

__EXPORT trax_handle* trax_client_setup(FILE* input, FILE* output, FILE* log);

__EXPORT int trax_client_wait(trax_handle* client, trax_region** region, trax_properties* properties);

__EXPORT void trax_client_initialize(trax_handle* client, trax_image* image, trax_region* region, trax_properties* properties);

__EXPORT void trax_client_frame(trax_handle* client, trax_image* image, trax_properties* properties);


/**
 *   
 *
**/
__EXPORT trax_handle* trax_server_setup_standard(trax_configuration config, FILE* log);

__EXPORT trax_handle* trax_server_setup(trax_configuration config, FILE* input, FILE* output, FILE* log);

__EXPORT int trax_server_wait(trax_handle* server, trax_image** image, trax_region** region, trax_properties* properties);

__EXPORT void trax_server_reply(trax_handle* server, trax_region* region, trax_properties* properties);


__EXPORT int trax_cleanup(trax_handle** handle);

__EXPORT void trax_image_release(trax_image** image);

__EXPORT trax_image* trax_image_create_path(const char* path);




__EXPORT void trax_region_release(trax_region** region);

__EXPORT int trax_region_get_type(const trax_region* region);

__EXPORT trax_region* trax_region_create_special(int code);

__EXPORT void trax_region_set_special(trax_region* region, int code);

__EXPORT int trax_region_get_special(const trax_region* region);

__EXPORT trax_region* trax_region_create_rectangle(float x, float y, float width, float height);

__EXPORT void trax_region_set_rectangle(trax_region* region, float x, float y, float width, float height);

__EXPORT void trax_region_get_rectangle(const trax_region* region, float* x, float* y, float* width, float* height);

__EXPORT trax_region* trax_region_create_polygon(int count);

__EXPORT void trax_region_set_polygon_point(trax_region* region, int index, float x, float y);

__EXPORT void trax_region_get_polygon_point(const trax_region* region, int index, float* x, float* y);

__EXPORT int trax_region_get_polygon_count(const trax_region* region);

/**
 * Creates a rectangle region object that bounds the input region (in case the input
 * region is also a rectangle it just clones it).
 **/
__EXPORT trax_region* trax_region_get_bounds(const trax_region* region);


__EXPORT void trax_properties_release(trax_properties** properties);

__EXPORT void trax_properties_clear(trax_properties* properties);

__EXPORT trax_properties* trax_properties_create();

__EXPORT void trax_properties_set(trax_properties* properties, const char* key, const char* value);

__EXPORT void trax_properties_set_int(trax_properties* properties, const char* key, int value);

__EXPORT void trax_properties_set_float(trax_properties* properties, const char* key, float value);

__EXPORT char* trax_properties_get(const trax_properties* properties, const char* key);

__EXPORT int trax_properties_get_int(const trax_properties* properties, const char* key, int def);

__EXPORT float trax_properties_get_float(const trax_properties* properties, const char* key, float def);

__EXPORT void trax_properties_enumerate(trax_properties* properties, trax_enumerator enumerator, void* object);

#ifdef __cplusplus
}
#endif

#endif

