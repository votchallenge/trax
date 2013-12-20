/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef TRAX_H
#define TRAX_H

#include <stdio.h>

#define TRAX_VERSION 1

#define TRAX_ERROR 0
#define TRAX_READY 1
#define TRAX_INIT 2
#define TRAX_FRAME 3
#define TRAX_QUIT 4
#define TRAX_STATUS 5

#define TRAX_IMAGE_PATH 0

#define TRAX_REGION_RECTANGLE 0

#define TRAX_FLAG_VALID 1
#define TRAX_FLAG_SERVER 2
#define TRAX_FLAG_LOG_INPUT 4
#define TRAX_FLAG_LOG_OUTPUT 8
#define TRAX_FLAG_LOG_DEBUG 16

#ifdef __cplusplus
extern "C" {
#endif

#define TRAX_PREFIX "@@TRAX:"
#define TRAX_PATH_MAX_LENGTH 1024

typedef struct trax_rectangle {
    float x;
    float y;
    float width;
    float height;
} trax_rectangle;

typedef char trax_imagepath[TRAX_PATH_MAX_LENGTH];

typedef struct trax_image {
    int type;
    union {
        trax_imagepath path;
    } data;
} trax_image;

typedef struct trax_region {
    int type;
    union {
        trax_rectangle rectangle;
    } data;
} trax_region;

typedef struct trax_configuration {
        int format_region;
        int format_image;
} trax_configuration;

typedef struct trax_handle trax_handle;

typedef struct trax_properties trax_properties;

typedef void(*trax_enumerator)(const char *key, const char *value, const void *obj);


void trax_test();

/**
 *
**/

trax_handle* trax_client_setup(FILE* input, FILE* output, FILE* log, int flags);

int trax_client_wait(trax_handle* client, trax_properties* properties, trax_region* region);

void trax_client_initialize(trax_handle* client, trax_image image, trax_region region, trax_properties* properties);

void trax_client_frame(trax_handle* client, trax_image image, trax_properties* properties);


/**
 *   
 *
**/
trax_handle* trax_server_setup(trax_configuration config, FILE* log, int flags);

int trax_server_wait(trax_handle* server, trax_image* image, trax_properties* properties, trax_region* region);

void trax_server_reply(trax_handle* server, trax_region region, trax_properties* properties);


int trax_cleanup(trax_handle** handle);

void trax_log(trax_handle* handle, const char *message);

void trax_properties_release(trax_properties** properties);

void trax_properties_clear(trax_properties* properties);

trax_properties* trax_properties_create();

void trax_properties_set(trax_properties* properties, const char* key, char* value);

void trax_properties_set_int(trax_properties* properties, const char* key, int value);

void trax_properties_set_float(trax_properties* properties, const char* key, float value);

char* trax_properties_get(trax_properties* properties, const char* key);

int trax_properties_get_int(trax_properties* properties, const char* key, int def);

float trax_properties_get_float(trax_properties* properties, const char* key, float def);

void trax_properties_enumerate(trax_properties* properties, trax_enumerator enumerator, void* object);

#ifdef __cplusplus
}
#endif

#endif

