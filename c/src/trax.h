/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef TRAX_H
#define TRAX_H

#include <stdio.h>

#define TRAX_ERROR 0
#define TRAX_READY 1
#define TRAX_INIT 2
#define TRAX_FRAME 3
#define TRAX_QUIT 4


#define TRAX_LOG_INPUT 1
#define TRAX_LOG_OUTPUT 2
#define TRAX_LOG_DEBUG 4

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

typedef struct trax_properties trax_properties;

typedef void(*trax_enumerator)(const char *key, const char *value, const void *obj);

typedef char trax_imagepath[TRAX_PATH_MAX_LENGTH];

void trax_test();

int trax_setup(const char* log, int flags);

int trax_cleanup();

int trax_wait(trax_imagepath path, trax_properties* properties, trax_rectangle * rectangle);

void trax_log(const char *message);

void trax_report_position(trax_rectangle position, trax_properties* properties);

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

