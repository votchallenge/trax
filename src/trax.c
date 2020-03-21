/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

//#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <float.h>

#define _TRAX_BUILDING

#include "trax.h"
#include "region.h"
#include "strmap.h"
#include "buffer.h"
#include "message.h"
#include "base64.h"
#include "debug.h"

#define VALIDATE_HANDLE(H) assert(((H)->flags & TRAX_FLAG_VALID))
#define VALIDATE_SERVER_HANDLE(H) assert(((H)->flags & TRAX_FLAG_VALID) && ((H)->flags & TRAX_FLAG_SERVER))
#define VALIDATE_CLIENT_HANDLE(H) assert(((H)->flags & TRAX_FLAG_VALID) && !((H)->flags & TRAX_FLAG_SERVER))

#define HANDLE_ALIVE(H) (((H)->flags & TRAX_FLAG_VALID) && !((H)->flags & TRAX_FLAG_TERMINATED))

#define LOGGER(H) (((H)->logging))

#define BUFFER_LENGTH 64
#define MAX_URI_SCHEME 16

#define COPY_ALL 1
#define COPY_EXTERNAL 0

#define REGION(VP) ((region_container*) (VP))

#define REGION_TYPE(VP) ( \
    (((region_container*) (VP))->type == RECTANGLE) ? TRAX_REGION_RECTANGLE : \
    (((region_container*) (VP))->type == POLYGON) ? TRAX_REGION_POLYGON : \
    (((region_container*) (VP))->type == MASK) ? TRAX_REGION_MASK : \
    (((region_container*) (VP))->type == SPECIAL) ? TRAX_REGION_SPECIAL : \
    EMPTY)

#define REGION_TYPE_BACK(T) (\
    ( T == TRAX_REGION_RECTANGLE ) ? RECTANGLE :\
    ( T == TRAX_REGION_POLYGON ) ? POLYGON :\
    ( T == TRAX_REGION_MASK ) ? MASK :\
    ( T == TRAX_REGION_SPECIAL ) ? SPECIAL :\
    EMPTY)

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <io.h>
int get_shared_fd(int h, int read) {
    return _open_osfhandle((intptr_t)h, read ? _O_RDONLY : 0);
}
#else
#define strcmpi strcasecmp
int get_shared_fd(int h, int read) {
    return h;
}
#endif

/* Visual Studio 2013 and earlier lack snprintf in their C99 library */
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define snprintf _snprintf
#endif


#ifndef TRAX_BUILD_VERSION
#define TRAX_BUILD_VERSION "unknown"
#endif

const char* trax_version() {
    return (const char*) (TRAX_BUILD_VERSION);
}

void file_logger(const char *string, int length, void* data) {
    if (!data) return;

    if (string)
        fwrite(string, sizeof(char), length, (FILE*) data);
    else
        fflush((FILE*) data);
}

const trax_logging trax_no_log = { 0, NULL, NULL };

const trax_bounds trax_no_bounds = { -FLT_MAX, FLT_MAX, -FLT_MAX, FLT_MAX };

trax_logging trax_logger_setup(trax_logger callback, void* data, int flags) {

    trax_logging logging;
    logging.callback = callback;
    logging.data = data;
    logging.flags = flags;
    return logging;

}

trax_logging trax_logger_setup_file(FILE* file) {

    return trax_logger_setup(file_logger, file, 0);

}

struct trax_properties {
    StrMap *map;
};

char* parse_uri(char* buffer) {
    int i = 0;

    for (; i < MAX_URI_SCHEME; i++) {
        if ((buffer[i] >= 'a' && buffer[i] <= 'z') || buffer[i] == '+' || buffer[i] == '.' || buffer[i] == '-') continue;

        if (buffer[i] == ':') {
            buffer[i] = 0; // Insert a stop char so that we can compare the prefix
            return buffer + i + 1;
        }

        return NULL;
    }

    return NULL;

}

int verify_image_format(const char* buffer) {

    int i;
    char jpeg_prefix[] = { (char) 255, (char) 216, (char) 255, (char) 224 };
    char png_prefix[] = { (char) 137, (char) 80, (char) 78, (char) 71 };

    for (i = 0; i < 4; i++) {
        if (buffer[i] != jpeg_prefix[i])
            goto png;
    }

    return TRAX_IMAGE_BUFFER_JPEG;

png:
    for (i = 0; i < 4; i++) {
        if (buffer[i] != png_prefix[i])
            goto end;
    }

    return TRAX_IMAGE_BUFFER_PNG;

end:

    return TRAX_IMAGE_BUFFER_ILLEGAL;

}

int decode_buffer_format(char* name) {

    if (strcmpi(name, "image/jpeg") == 0)
        return TRAX_IMAGE_BUFFER_JPEG;

    if (strcmpi(name, "image/png") == 0)
        return TRAX_IMAGE_BUFFER_PNG;

    return TRAX_IMAGE_BUFFER_ILLEGAL;

}


int decode_memory_format(char* name) {

    if (strcmpi(name, "rgb") == 0)
        return TRAX_IMAGE_MEMORY_RGB;

    if (strcmpi(name, "gray8") == 0)
        return TRAX_IMAGE_MEMORY_GRAY8;

    if (strcmpi(name, "gray16") == 0)
        return TRAX_IMAGE_MEMORY_GRAY16;

    return TRAX_IMAGE_MEMORY_ILLEGAL;

}


int compare_prefix(char* str, const char* prefix) {
    int i = 0;

    while (1) {

        if (str[i] == prefix[i]) {
            if (!prefix[i]) return 1;
            i++;
        } else return prefix[i] == 0;

    }

}

char* strntok(char* str, char query, int limit) {
    int i = 0;

    for (i = 0; i < limit; i++) {

        if (str[i] == query) {
            str[i] = 0;
            return str + i + 1;
        }

        if (!str[i]) return NULL;

    }

    return NULL;

}

void copy_property_all(const char *key, const char *value, const void *obj) {

    trax_properties* dest = (trax_properties*) obj;
    trax_properties_set(dest, key, value);

}

void copy_property_external(const char *key, const char *value, const void *obj) {

    trax_properties* dest = (trax_properties*) obj;

    if (strncmp(key, "trax.", 5) == 0) return;

    trax_properties_set(dest, key, value);

}

char* image_encode(trax_image* image) {

    char* result = NULL;

    switch (image->type) {
    case TRAX_IMAGE_PATH: {
        int length = strlen(image->data);
        result = (char*) malloc(sizeof(char) * (length + 1 + 7));
        sprintf(result, "file://");
        memcpy(result + 7, image->data, length + 1);
        break;
    }
    case TRAX_IMAGE_URL: {
        int length = strlen(image->data);
        result = (char*) malloc(sizeof(char) * (length + 1));
        memcpy(result, image->data, length + 1);
        break;
    }
    case TRAX_IMAGE_MEMORY: {
        int offset = 0;
        const char* format = image->format == TRAX_IMAGE_MEMORY_RGB ? "rgb" :
                             image->format == TRAX_IMAGE_MEMORY_GRAY8 ? "gray8" :
                             image->format == TRAX_IMAGE_MEMORY_GRAY16 ? "gray16" : NULL;
        int depth = image->format == TRAX_IMAGE_MEMORY_RGB ? 1 :
                    (image->format == TRAX_IMAGE_MEMORY_GRAY8 ? 1 :
                     (image->format == TRAX_IMAGE_MEMORY_GRAY16 ? 2 : 0));
        int channels = image->format == TRAX_IMAGE_MEMORY_RGB ? 3 : 1;
        int length = (image->width * image->height * depth * channels);
        int encoded = base64encodelen(length);
        int header = snprintf(NULL, 0, "image:%d;%d;%s;", image->width, image->height, format);
        assert(format);
        result = (char*) malloc(sizeof(char) * (encoded + header + 1));
        offset += sprintf(result, "image:%d;%d;%s;", image->width, image->height, format);
        base64encode(result + offset, (unsigned char*)image->data, length);
        break;
    }
    case TRAX_IMAGE_BUFFER: {
        int offset = 0;
        int length = image->width;
        const char* format = (image->format == TRAX_IMAGE_BUFFER_JPEG) ? "image/jpeg" :
                             ((image->format == TRAX_IMAGE_BUFFER_PNG) ? "image/png" : NULL);
        int body = base64encodelen(length);
        int header = snprintf(NULL, 0, "data:%s;", format);
        assert(format);
        result = (char*) malloc(sizeof(char) * (body + header + 1));
        offset += sprintf(result, "data:%s;", format);
        offset += base64encode(result + offset, (unsigned char*) image->data, length);
        //result[offset] = 0;
        break;
    }

    }

    return result;
}

trax_image* image_decode(char* buffer) {

    trax_image* result = NULL;

    char* resource = parse_uri(buffer);

    if (!resource) {
        // Support for legacy format
        result = trax_image_create_path(buffer);
        return result;
    }

    if (strcmp(buffer, "file") == 0) {
        result = trax_image_create_path(buffer + (compare_prefix(resource, "//") ? 7 : 5));
    } else if (strcmp(buffer, "image") == 0) {
        int outlen, width, height, depth, format, channels, allocated, verify;
        char* token;

        width = strtol(resource, &resource, 10);
        if (resource[0] != ';') return result;
        height = strtol(resource + 1, &resource, 10);
        if (resource[0] != ';') return result;
        token = resource + 1;
        resource = strntok(token, ';', 32);

        if (!resource) return NULL;
        format = decode_memory_format(token);

        outlen = base64decodelen(resource) - 1;

        depth = format == TRAX_IMAGE_MEMORY_RGB ? 1 :
                (format == TRAX_IMAGE_MEMORY_GRAY8 ? 1 :
                 (format == TRAX_IMAGE_MEMORY_GRAY16 ? 2 : 0));
        channels = format == TRAX_IMAGE_MEMORY_RGB ? 3 : 1;
        allocated =  (width * height * depth * channels);

        if (width < 1 || height < 1 || outlen != allocated) return result;

        result = (trax_image*) malloc(sizeof(trax_image));
        result->type = TRAX_IMAGE_MEMORY;
        result->width = width;
        result->height = height;
        result->format = format;
        result->data = (char*) malloc(sizeof(char) * allocated);
        verify = base64decode((unsigned char*)result->data, resource);

        assert(verify == allocated);

    } else if (strcmp(buffer, "data") == 0) {
        int format, outlen;
        char* token;

        token = resource;
        resource = strntok(token, ';', 32);
        if (!resource) return NULL;
        format = decode_buffer_format(token);
        outlen = base64decodelen(resource);

        result = (trax_image*) malloc(sizeof(trax_image));
        result->type = TRAX_IMAGE_BUFFER;
        result->width = outlen;
        result->height = 1;
        result->format = format;

        result->data = (char*) malloc(sizeof(char) * (outlen));
        base64decode((unsigned char*)result->data, resource);
    } else {
        *(resource--) = ':'; // Restore the semicolon and use the buffer as URL
        result = trax_image_create_url(buffer);
    }
    return result;

}

int image_formats_decode(char *str) {

    int formats = 0;

    char *pch;
    pch = strtok (str, " ;");
    while (pch != NULL) {
        if (strcmp(pch, "path") == 0)
            formats |= TRAX_IMAGE_PATH;
        else if (strcmp(pch, "url") == 0)
            formats |= TRAX_IMAGE_URL;
        else if (strcmp(pch, "memory") == 0)
            formats |= TRAX_IMAGE_MEMORY;
        else if (strcmp(pch, "buffer") == 0)
            formats |= TRAX_IMAGE_BUFFER;
        else if (strlen(pch) == 0)
            continue; // Skip empty
        else return -1;

        pch = strtok (NULL, ";");
    }

    return formats;
}

void image_formats_encode(int formats, char *key) {

    char* pch = key;

    pch[0] = 0;

    if (TRAX_SUPPORTS(formats, TRAX_IMAGE_PATH)) {
        pch += sprintf(pch, "path;");
    }
    if (TRAX_SUPPORTS(formats, TRAX_IMAGE_URL)) {
        pch += sprintf(pch, "url;");
    }
    if (TRAX_SUPPORTS(formats, TRAX_IMAGE_MEMORY)) {
        pch += sprintf(pch, "memory;");
    }
    if (TRAX_SUPPORTS(formats, TRAX_IMAGE_BUFFER)) {
        pch += sprintf(pch, "buffer;");
    }

}

int region_formats_decode(char *str) {

    int formats = 0;

    char *pch;
    pch = strtok (str, " ;");
    while (pch != NULL) {

        if (strcmp(pch, "rectangle") == 0)
            formats |= TRAX_REGION_RECTANGLE;
        else if (strcmp(pch, "polygon") == 0)
            formats |= TRAX_REGION_POLYGON;
        else if (strcmp(pch, "mask") == 0)
            formats |= TRAX_REGION_MASK;
        else if (strlen(pch) == 0)
            continue; // Skip empty
        else return -1;

        pch = strtok (NULL, ";");
    }

    return formats;
}

void region_formats_encode(int formats, char *key) {

    char* pch = key;

    pch[0] = 0;

    if (TRAX_SUPPORTS(formats, TRAX_REGION_RECTANGLE)) {
        pch += sprintf(pch, "rectangle;");
    }
    if (TRAX_SUPPORTS(formats, TRAX_REGION_POLYGON)) {
        pch += sprintf(pch, "polygon;");
    }
    if (TRAX_SUPPORTS(formats, TRAX_REGION_MASK)) {
        pch += sprintf(pch, "mask;");
    }

}

int channels_decode(char *str) {

    int channels = 0;

    char *pch;
    pch = strtok (str, " ;");
    while (pch != NULL) {

        if (strcmp(pch, "color") == 0)
            channels |= TRAX_CHANNEL_COLOR;
        else if (strcmp(pch, "depth") == 0)
            channels |= TRAX_CHANNEL_DEPTH;
        else if (strcmp(pch, "ir") == 0)
            channels |= TRAX_CHANNEL_IR;

        else if (strlen(pch) == 0)
            continue; // Skip empty
        else return -1;

        pch = strtok (NULL, ";");
    }

    return channels;
}

void channels_encode(int channels, char *key) {

    char* pch = key;

    pch[0] = 0;

    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_COLOR)) {
        pch += sprintf(pch, "color;");
    }
    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_DEPTH)) {
        pch += sprintf(pch, "depth;");
    }
    if (TRAX_SUPPORTS(channels, TRAX_CHANNEL_IR)) {
        pch += sprintf(pch, "ir;");
    }

}

void copy_properties(const trax_properties* source, trax_properties* dest, int internal) {

    trax_properties_enumerate(source, (internal) ? copy_property_all : copy_property_external, dest);

}

void clear_error(trax_handle* handle) {

    if (handle->error) {
        free(handle->error);
        handle->error = NULL;
    }

}

void set_error(trax_handle* handle, const char *format, ...) {
	va_list args;

    string_buffer* buffer = buffer_create(100);

	va_start(args, format);
	
    buffer_append(buffer, format, args);

	va_end(args);

    if (handle->error) {
        free(handle->error);
    }

    handle->error = buffer_extract(buffer);

    buffer_destroy(&buffer);

}


trax_handle* client_setup(message_stream* stream, const trax_logging log) {

    trax_properties* tmp_properties;
    string_list* arguments;
    int version = 1;
    char *tmp, *tracker_name, *tracker_description, *tracker_family;
    int region_formats, image_formats, channels;

    trax_handle* client = (trax_handle*) malloc(sizeof(trax_handle));

    client->flags = (0 & ~TRAX_FLAG_SERVER) | TRAX_FLAG_VALID;

    client->logging = log;
    client->stream = stream;
    client->error = NULL;

    tmp_properties = trax_properties_create();
    arguments = list_create(8);

    if (read_message((message_stream*)client->stream, &LOGGER(client), arguments, tmp_properties) != TRAX_HELLO) {
        DEBUGMSG("Unable to parse hello message.");
        goto failure;
    }

    if (list_size(arguments) > 0) {
        DEBUGMSG("Illegal number of arguments %d, required more.", list_size(arguments));
        goto failure;
    }

    client->version = trax_properties_get_int(tmp_properties, "trax.version", 1);

    tmp = trax_properties_get(tmp_properties, "trax.region");
    region_formats = region_formats_decode(tmp);
    free(tmp);

    tmp = trax_properties_get(tmp_properties, "trax.image");
    image_formats = image_formats_decode(tmp);
    free(tmp);

    // Multiple channels are only supported in TraX version 2
    if (client->version < 2) {
        channels = TRAX_CHANNEL_COLOR;
    } else {
        tmp = trax_properties_get(tmp_properties, "trax.channels");
        channels = channels_decode(tmp);
        free(tmp);
        if (channels == 0)
            channels = TRAX_CHANNEL_COLOR;
    }

    tracker_name = trax_properties_get(tmp_properties, "trax.name");
    tracker_description = trax_properties_get(tmp_properties, "trax.description");
    tracker_family = trax_properties_get(tmp_properties, "trax.family");

    client->metadata = trax_metadata_create(region_formats, image_formats, channels,
                                            tracker_name, tracker_description, tracker_family);


    if (tracker_name) free(tracker_name);
    if (tracker_description) free(tracker_description);
    if (tracker_family) free(tracker_family);

    copy_properties(tmp_properties, client->metadata->custom, COPY_EXTERNAL);

    trax_properties_release(&tmp_properties);
    list_destroy(&arguments);

    return client;

failure:

    list_destroy(&arguments);
    trax_properties_release(&tmp_properties);
    free(client);
    return NULL;

}

trax_handle* server_setup(trax_metadata *metadata, message_stream* stream, const trax_logging log) {

    trax_properties* properties;
    trax_handle* server = (trax_handle*) malloc(sizeof(trax_handle));
    string_list* arguments;
    char tmp[BUFFER_LENGTH];

    server->flags = (TRAX_FLAG_SERVER) | TRAX_FLAG_VALID;
    server->logging = log;
    server->error = NULL;
    server->stream = stream;

    if (metadata->custom) {
        properties = trax_properties_copy(metadata->custom);
    } else {
        properties = trax_properties_create();
    }
    
    server->version = TRAX_VERSION;
    trax_properties_set_int(properties, "trax.version", TRAX_VERSION);

    region_formats_encode(metadata->format_region, tmp);
    trax_properties_set(properties, "trax.region", tmp);

    image_formats_encode(metadata->format_image, tmp);
    trax_properties_set(properties, "trax.image", tmp);

    channels_encode(metadata->channels, tmp);
    trax_properties_set(properties, "trax.channels", tmp);

    if (metadata->tracker_name)
        trax_properties_set(properties, "trax.name", metadata->tracker_name);

    if (metadata->tracker_description)
        trax_properties_set(properties, "trax.description", metadata->tracker_description);

    if (metadata->tracker_family)
        trax_properties_set(properties, "trax.family", metadata->tracker_family);


    server->metadata = trax_metadata_create(metadata->format_region, metadata->format_image, metadata->channels,
                                            metadata->tracker_name, metadata->tracker_description, metadata->tracker_family);

    arguments = list_create(1);

    write_message((message_stream*)server->stream, &LOGGER(server), TRAX_HELLO, arguments, properties);

    trax_properties_release(&properties);

    list_destroy(&arguments);

    return server;

}

trax_metadata* trax_metadata_create(int region_formats, int image_formats, int channels,
                                    const char* tracker_name, const char* tracker_description, const char* tracker_family) {

    assert(channels != 0);

    trax_metadata* metadata = (trax_metadata*) malloc(sizeof(trax_metadata));

    metadata->format_region = region_formats;
    metadata->format_image = image_formats;
    metadata->channels = channels;

    metadata->tracker_name = (tracker_name) ? strdup(tracker_name) : NULL;
    metadata->tracker_description = (tracker_description) ? strdup(tracker_description) : NULL;
    metadata->tracker_family = (tracker_family) ? strdup(tracker_family) : NULL;

    metadata->custom = trax_properties_create();

    return metadata;

}

void trax_metadata_release(trax_metadata** metadata) {

    if (!*metadata) return;

    if ((*metadata)->tracker_name) free((*metadata)->tracker_name);
    if ((*metadata)->tracker_description) free((*metadata)->tracker_description);
    if ((*metadata)->tracker_family) free((*metadata)->tracker_family);

    if ((*metadata)->custom) trax_properties_release(&(*metadata)->custom);

    free(*metadata);

    *metadata = NULL;

}


trax_handle* trax_client_setup_file(int input, int output, const trax_logging log) {

    message_stream* stream = create_message_stream_file(input, output);

    return client_setup(stream, log);

}

trax_handle* trax_client_setup_socket(int server, int timeout, const trax_logging log) {

    message_stream* stream = create_message_stream_socket_accept(server, timeout);

    if (!stream) return NULL;

    return client_setup(stream, log);

}

int trax_client_wait(trax_handle* client, trax_region** region, trax_properties* properties) {

    trax_properties* tmp_properties;
    string_list* arguments;
    int result = TRAX_ERROR;

    (*region) = NULL;

    VALIDATE_CLIENT_HANDLE(client);

    if (!HANDLE_ALIVE(client))
        return TRAX_ERROR;

    tmp_properties = trax_properties_create();
    arguments = list_create(8);

    result = read_message((message_stream*)client->stream, &LOGGER(client), arguments, tmp_properties);

    if (result == TRAX_STATE) {

        region_container *_region = NULL;

        if (list_size(arguments) != 1)
            goto failure;

        result = TRAX_STATE;

        if (!region_parse(arguments->buffer[0], &_region)) {
            goto failure;
        }

        (*region) = _region;

        copy_properties(tmp_properties, properties, 1);

        goto end;

    } else if (result == TRAX_QUIT) {

        if (list_size(arguments) != 0)
            goto failure;

        if (properties)
            copy_properties(tmp_properties, properties, 1);

        client->flags |= TRAX_FLAG_TERMINATED;

        goto end;
    }

failure:
    result = TRAX_ERROR;

end:

    list_destroy(&arguments);
    trax_properties_release(&tmp_properties);

    return result;

}

int trax_client_initialize(trax_handle* client, trax_image_list* images, trax_region* region, trax_properties* properties) {

    char* data = NULL;
    region_container* _region;
    string_list* arguments;
    int i;

    VALIDATE_CLIENT_HANDLE(client);

    clear_error(client);

    if (!HANDLE_ALIVE(client)) {
        set_error(client, "Client not alive");
        return TRAX_ERROR;
    }

    assert(images && region);

    _region = REGION(region);

    assert(_region->type != SPECIAL);

    arguments = list_create(1);

    for (i = 0; i < TRAX_CHANNELS; i++) {
        if (TRAX_SUPPORTS(client->metadata->channels, TRAX_CHANNEL_ID(i))) {

            if (!images->images[i]) {
                set_error(client, "Required image channel not provided (ID: %d)", TRAX_CHANNEL_ID(i));
                goto failure;
            }
            char* buffer = image_encode(images->images[i]);
            list_append_direct(arguments, buffer);

        }
    }

    if (!TRAX_SUPPORTS(client->metadata->format_region, REGION_TYPE(region))) {

        trax_region* converted = NULL;

        if TRAX_SUPPORTS(client->metadata->format_region, TRAX_REGION_MASK)
            converted = region_convert((region_container *)region, REGION_TYPE_BACK(TRAX_REGION_MASK));
        else if TRAX_SUPPORTS(client->metadata->format_region, TRAX_REGION_POLYGON)
            converted = region_convert((region_container *)region, REGION_TYPE_BACK(TRAX_REGION_POLYGON));
        else if TRAX_SUPPORTS(client->metadata->format_region, TRAX_REGION_RECTANGLE)
            converted = region_convert((region_container *)region, REGION_TYPE_BACK(TRAX_REGION_RECTANGLE));

        assert(converted);

        data = region_string((region_container *)converted);

        trax_region_release(&converted);

    } else data = region_string((region_container *)region);

    if (data) {
        list_append(arguments, data);
        free(data);
    }

    write_message((message_stream*)client->stream, &LOGGER(client), TRAX_INITIALIZE, arguments, properties);

    list_destroy(&arguments);

    return TRAX_OK;

failure:

    list_destroy(&arguments);

    return TRAX_ERROR;

}

int trax_client_frame(trax_handle* client, trax_image_list* images, trax_properties* properties) {

    int i;
    string_list* arguments;
    VALIDATE_CLIENT_HANDLE(client);

    clear_error(client);

    if (!HANDLE_ALIVE(client)) {
        set_error(client, "Client not alive");
        return TRAX_ERROR;
    }

    arguments = list_create(1);

    for (i = 0; i < TRAX_CHANNELS; i++) {
        if (TRAX_SUPPORTS(client->metadata->channels, TRAX_CHANNEL_ID(i))) {
            if (!images->images[i]) {
                set_error(client, "Required image channel not provided (ID: %d)", TRAX_CHANNEL_ID(i));
                goto failure;
            }
            char* buffer = image_encode(images->images[i]);
            list_append_direct(arguments, buffer);
        } 
    }

    write_message((message_stream*)client->stream, &LOGGER(client), TRAX_FRAME, arguments, properties);

    list_destroy(&arguments);

    return TRAX_OK;

failure:

    list_destroy(&arguments);

    return TRAX_ERROR;

}

trax_handle* trax_server_setup(trax_metadata *metadata, const trax_logging log) {

    message_stream* stream;

    char* env_socket = getenv("TRAX_SOCKET");

    if (env_socket) {

        stream = create_message_stream_socket_connect(atoi(env_socket));

    } else {

        int fin = fileno(stdin);
        int fout = fileno(stdout);

        char* env_in = getenv("TRAX_IN");
        char* env_out = getenv("TRAX_OUT");

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(_MSC_VER)
        if (env_in) fin = get_shared_fd(strtol(env_in, NULL, 16), 1);
        if (env_out) fout = get_shared_fd(strtol(env_out, NULL, 16), 0);
#else
        if (env_in) fin = get_shared_fd(atoi(env_in), 1);
        if (env_out) fout = get_shared_fd(atoi(env_out), 0);
#endif

        stream = create_message_stream_file(fin, fout);

    }

    return server_setup(metadata, stream, log);

}

trax_handle* trax_server_setup_file(trax_metadata *metadata, int input, int output, const trax_logging log) {

    message_stream* stream = create_message_stream_file(input, output);

    return server_setup(metadata, stream, log);
}

int trax_server_wait(trax_handle* server, trax_image_list** images, trax_region** region, trax_properties* properties) {

    int result = TRAX_ERROR;
    int i, j = 0;
    string_list* arguments;
    trax_properties* tmp_properties;

    VALIDATE_SERVER_HANDLE(server);

    clear_error(server);

    if (!HANDLE_ALIVE(server)) {
        set_error(server, "Server not alive");
        return TRAX_ERROR;
    }

    int argument_count = trax_image_list_count(server->metadata->channels);

    tmp_properties = trax_properties_create();
    arguments = list_create(8);

    result = read_message((message_stream*)server->stream, &LOGGER(server), arguments, tmp_properties);

    if (result == TRAX_FRAME) {

        if (list_size(arguments) != argument_count) {
            set_error(server, "Protocol error, illegal argument number %d != %d", list_size(arguments), argument_count);
            goto failure;
        }   
            
        *images = trax_image_list_create();

        for (i = 0; i < TRAX_CHANNELS; i++) {

            if (TRAX_SUPPORTS(server->metadata->channels, TRAX_CHANNEL_ID(i))) {

                (*images)->images[i] = image_decode(arguments->buffer[j]);
                if (!(*images)->images[i] || !TRAX_SUPPORTS(server->metadata->format_image, (*images)->images[i]->type))
                    goto failure;
                j++;

            }

        }

        if (properties)
            copy_properties(tmp_properties, properties, COPY_ALL);
        goto end;

    } else if (result == TRAX_QUIT) {

        if (list_size(arguments) != 0){
            set_error(server, "Protocol error, illegal argument number %d != %d", list_size(arguments), 0);
            goto failure;
        }   

        if (properties)
            copy_properties(tmp_properties, properties, COPY_ALL);

        server->flags |= TRAX_FLAG_TERMINATED;

        goto end;

    } else if (result == TRAX_INITIALIZE) {

        if (list_size(arguments) != (argument_count + 1)) { // There's also the region info
            set_error(server, "Protocol error, illegal argument number %d != %d", list_size(arguments), argument_count + 1);
            goto failure;
        }    

        *images = trax_image_list_create();

        for (i = 0; i < TRAX_CHANNELS; i++) {

            if (TRAX_SUPPORTS(server->metadata->channels, TRAX_CHANNEL_ID(i))) {

                (*images)->images[i] = image_decode(arguments->buffer[j]);
                if (!(*images)->images[i] || !TRAX_SUPPORTS(server->metadata->format_image, (*images)->images[i]->type))
                    goto failure;
                j++;

            }

        }

        if (!region_parse(arguments->buffer[j], (region_container**)region)) {
            goto failure;
        }

        if (properties)
            copy_properties(tmp_properties, properties, COPY_ALL);

        goto end;
    }

failure:

    result = TRAX_ERROR;

    if (*images) {
        for (i = 0; i < TRAX_CHANNELS; i++) {
            if ((*images)->images[i]) trax_image_release(&(*images)->images[i]);
        }
        trax_image_list_release(images);
    }

    if (*region)
        trax_region_release(region);

end:

    list_destroy(&arguments);
    trax_properties_release(&tmp_properties);

    return result;
}

int trax_server_reply(trax_handle* server, trax_region* region, trax_properties* properties) {

    char* data;
    string_list* arguments;

    VALIDATE_SERVER_HANDLE(server);

    clear_error(server);

    if (!HANDLE_ALIVE(server)) {
        set_error(server, "Server not alive");
        return TRAX_ERROR;
    }

    data = region_string(REGION(region));

    if (!data) return TRAX_ERROR;

    arguments = list_create(1);

    list_append_direct(arguments, data);

    write_message((message_stream*)server->stream, &LOGGER(server), TRAX_STATE, arguments, properties);

    list_destroy(&arguments);

    return TRAX_OK;

}

const char* trax_get_error(trax_handle* handle) {

    VALIDATE_HANDLE(handle);

    return handle->error;

}

int trax_is_alive(trax_handle* handle) {

    VALIDATE_HANDLE(handle);

    return HANDLE_ALIVE(handle);

}

int trax_terminate(trax_handle* handle, const char* reason) {

    string_list *arguments;
    trax_properties* tmp_properties;

    VALIDATE_HANDLE(handle);

    clear_error(handle);

    if (!HANDLE_ALIVE(handle)) 
        return TRAX_OK;

    arguments = list_create(1);
    tmp_properties = trax_properties_create();

    if (reason)
        trax_properties_set(tmp_properties, "trax.reason", reason);

    write_message((message_stream*)handle->stream, &LOGGER(handle), TRAX_QUIT, arguments, tmp_properties);

    list_destroy(&arguments);
    trax_properties_release(&tmp_properties);

    handle->flags |= TRAX_FLAG_TERMINATED;

    return TRAX_OK;

}

int trax_cleanup(trax_handle** handle) {

    if (!*handle) return -1;

    VALIDATE_HANDLE((*handle));

    trax_terminate((*handle), NULL);

    trax_metadata_release(&(*handle)->metadata);

    destroy_message_stream((message_stream**) & (*handle)->stream);

    clear_error(*handle);

    free(*handle);
    *handle = 0;

    return 0;
}

int trax_set_parameter(trax_handle* handle, int id, int value) {

    if (!HANDLE_ALIVE(handle))
        return TRAX_ERROR;

    // No settable parameters at the moment.

    return 0;
}

int trax_get_parameter(trax_handle* handle, int id, int* value) {

    if (!HANDLE_ALIVE(handle))
        return TRAX_ERROR;

    switch (id) {
    case TRAX_PARAMETER_VERSION:
        *value = handle->version;
        return 1;
    case TRAX_PARAMETER_CLIENT:
        *value = !(handle->flags & TRAX_FLAG_SERVER);
        return 1;
    case TRAX_PARAMETER_SOCKET:
        *value = (((message_stream*)handle->stream)->flags & TRAX_STREAM_SOCKET);
        return 1;
    }

    return 0;
}

void trax_image_release(trax_image** image) {

    if ((*image)->data) {
        free((*image)->data);
        (*image)->data = 0;
    }

    free(*image);

    *image = NULL;
}

trax_image* trax_image_create_path(const char* path) {

    trax_image* img = (trax_image*) malloc(sizeof(trax_image));

    img->type = TRAX_IMAGE_PATH;
    img->width = 0;
    img->height = 0;
    img->data = (char*) malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(img->data, path);

    return img;
}

trax_image* trax_image_create_url(const char* url) {

    trax_image* img = (trax_image*) malloc(sizeof(trax_image));

    img->type = TRAX_IMAGE_URL;
    img->width = 0;
    img->height = 0;
    img->data = (char*) malloc(sizeof(char) * (strlen(url) + 1));
    strcpy(img->data, url);

    return img;

}

trax_image* trax_image_create_memory(int width, int height, int format) {

    int channels, depth;
    trax_image* img;

    assert(format == TRAX_IMAGE_MEMORY_GRAY8 ||
           format == TRAX_IMAGE_MEMORY_GRAY16 || format == TRAX_IMAGE_MEMORY_RGB);

    depth = format == TRAX_IMAGE_MEMORY_RGB ? 1 :
            (format == TRAX_IMAGE_MEMORY_GRAY8 ? 1 :
             (format == TRAX_IMAGE_MEMORY_GRAY16 ? 2 : 0));
    channels = format == TRAX_IMAGE_MEMORY_RGB ? 3 : 1;

    img = (trax_image*) malloc(sizeof(trax_image));

    img->type = TRAX_IMAGE_MEMORY;
    img->width = width;
    img->height = height;
    img->format = format;
    img->data = (char*) malloc(sizeof(char) * (width * height * depth * channels));

    return img;

}

trax_image* trax_image_create_buffer(int length, const char* data) {

    int format;
    trax_image* img = NULL;

    assert(length > 4);
    format = verify_image_format(data);
    assert(format != TRAX_IMAGE_BUFFER_ILLEGAL);

    img = (trax_image*) malloc(sizeof(trax_image));

    img->type = TRAX_IMAGE_BUFFER;
    img->width = length;
    img->height = 1;
    img->format = format;
    img->data = (char*) malloc(sizeof(char) * length);
    memcpy(img->data, data, length);

    return img;

}

int trax_image_get_type(const trax_image* image) {

    if (!image) return TRAX_IMAGE_EMPTY;

    return image->type;

}

const char* trax_image_get_path(const trax_image* image) {

    assert(image->type == TRAX_IMAGE_PATH);

    return image->data;

}

const char* trax_image_get_url(const trax_image* image) {

    assert(image->type == TRAX_IMAGE_URL);

    return image->data;
}

void trax_image_get_memory_header(const trax_image* image, int* width, int* height, int* format) {

    assert(image->type == TRAX_IMAGE_MEMORY);

    *width = image->width;
    *height = image->height;
    *format = image->format;

}

#define MEMORY_DEPTH(image) (image->format == TRAX_IMAGE_MEMORY_RGB ? 1 : \
    (image->format == TRAX_IMAGE_MEMORY_GRAY8 ? 1 : \
    (image->format == TRAX_IMAGE_MEMORY_GRAY16 ? 2 : 0))) \

#define MEMORY_CHANNELS(image) (image->format == TRAX_IMAGE_MEMORY_RGB ? 3 : 1)

char* trax_image_write_memory_row(trax_image* image, int row) {

    int depth, channels;

    assert(image->type == TRAX_IMAGE_MEMORY);
    assert(row >= 0 && row < image->height);

    depth = MEMORY_DEPTH(image);
    channels = MEMORY_CHANNELS(image);

    return &(image->data[depth * channels * row]);
}

const char* trax_image_get_memory_row(const trax_image* image, int row) {

    int depth, channels;

    assert(image->type == TRAX_IMAGE_MEMORY);
    assert(row >= 0 && row < image->height);

    depth = MEMORY_DEPTH(image);
    channels = MEMORY_CHANNELS(image);

    return &(image->data[depth * channels * row]);
}

const char* trax_image_get_buffer(const trax_image* image, int* length, int* format) {

    assert(image->type == TRAX_IMAGE_BUFFER);

    *length = image->width;
    *format = image->format;

    return image->data;

}

void trax_region_release(trax_region** region) {

    region_container* _region = REGION(*region);

    region_release(&_region);

    *region = NULL;

}

trax_region* trax_region_create_special(int code) {

    return region_create_special(code);

}

trax_region* trax_region_create_polygon(int size) {

    assert(size > 2);

    return region_create_polygon(size);

}

trax_region* trax_region_create_rectangle(float x, float y, float width, float height) {

    return region_create_rectangle(x, y, width, height);

}

trax_bounds trax_region_bounds(const trax_region* region) {

    trax_bounds tb;
    region_bounds rb = region_compute_bounds(REGION(region));

    tb.top = rb.top;
    tb.left = rb.left;
    tb.right = rb.right;
    tb.bottom = rb.bottom;

    return tb;

}

trax_region* trax_region_clone(const trax_region* region) {

    if (!region) return NULL;

    return region_convert(REGION(region), REGION_TYPE_BACK(trax_region_get_type(region)));

}

trax_region* trax_region_convert(const trax_region* region, int format) {

    if (!region) return NULL;

    return region_convert(REGION(region), REGION_TYPE_BACK(format));

}

float trax_region_overlap(const trax_region* a, const trax_region* b, const trax_bounds bounds) {

    region_bounds rb;

    if (!a || !b) return 0;

    rb.top = bounds.top;
    rb.left = bounds.left;
    rb.right = bounds.right;
    rb.bottom = bounds.bottom;

    return region_compute_overlap(REGION(a), REGION(b), rb).overlap;

}

trax_region* trax_region_get_bounds(const trax_region* region) {

    return region_convert(REGION(region), RECTANGLE);

}

int trax_region_contains(const trax_region* region, float x, float y) {

    return region_contains_point(REGION(region), x, y);

}

int trax_region_get_type(const trax_region* region) {

    if (!region) return TRAX_REGION_EMPTY;

    return REGION_TYPE(region);

}

void trax_region_set_special(trax_region* region, int code) {

    assert(REGION(region)->type == SPECIAL);

    REGION(region)->data.special = (int) code;

}


int trax_region_get_special(const trax_region* region) {

    assert(REGION(region)->type == SPECIAL);

    return REGION(region)->data.special;

}

void trax_region_set_rectangle(trax_region* region, float x, float y, float width, float height) {

    assert(REGION(region)->type == RECTANGLE);

    REGION(region)->data.rectangle.x = x;
    REGION(region)->data.rectangle.y = y;
    REGION(region)->data.rectangle.width = width;
    REGION(region)->data.rectangle.height = height;

}

void trax_region_get_rectangle(const trax_region* region, float* x, float* y, float* width, float* height) {

    assert(REGION(region)->type == RECTANGLE);

    if (x) *x = REGION(region)->data.rectangle.x;
    if (y) *y = REGION(region)->data.rectangle.y;
    if (width) *width = REGION(region)->data.rectangle.width;
    if (height) *height = REGION(region)->data.rectangle.height;

}

void trax_region_set_polygon_point(trax_region* region, int index, float x, float y) {

    assert(REGION(region)->type == POLYGON);

    assert(index >= 0 || index < (REGION(region)->data.polygon.count));

    REGION(region)->data.polygon.x[index] = x;
    REGION(region)->data.polygon.y[index] = y;
}

void trax_region_get_polygon_point(const trax_region* region, int index, float* x, float* y) {

    assert(REGION(region)->type == POLYGON);

    assert (index >= 0 || index < (REGION(region)->data.polygon.count));

    if (x) *x = REGION(region)->data.polygon.x[index];
    if (y) *y = REGION(region)->data.polygon.y[index];
}

int trax_region_get_polygon_count(const trax_region* region) {

    assert(REGION(region)->type == POLYGON);

    return REGION(region)->data.polygon.count;

}

trax_region* trax_region_create_mask(int x, int y, int width, int height) {

    return region_create_mask(x, y, width, height);

}

void trax_region_get_mask_header(const trax_region* region, int* x, int* y, int* width, int* height) {

    assert(REGION(region)->type == MASK);

    if (x) *x = REGION(region)->data.mask.x;
    if (y) *y = REGION(region)->data.mask.y;
    if (width) *width = REGION(region)->data.mask.width;
    if (height) *height = REGION(region)->data.mask.height;

}

char* trax_region_write_mask_row(trax_region* region, int row) {

    assert(REGION(region)->type == MASK);

    assert(row >= 0 && row < REGION(region)->data.mask.height);

    return &(REGION(region)->data.mask.data[REGION(region)->data.mask.width * row]);

}

const char* trax_region_get_mask_row(const trax_region* region, int row) {

    assert(REGION(region)->type == MASK);

    assert(row >= 0 && row < REGION(region)->data.mask.height);

    return &(REGION(region)->data.mask.data[REGION(region)->data.mask.width * row]);
    
}

char* trax_region_encode(const trax_region* region) {

    return region_string(REGION(region));

}

trax_region* trax_region_decode(const char* data) {

    region_container* region;

    if (!region_parse(data, &region))
        return NULL;

    return (trax_region*) region;

}

void trax_properties_release(trax_properties** properties) {

    if (properties && *properties) {
        if ((*properties)->map) sm_delete((*properties)->map);
        free((*properties));
        *properties = 0;
    }

}

void trax_properties_clear(trax_properties* properties) {

    if (properties) {
        if (properties->map)
            sm_delete(properties->map);
        properties->map = sm_new(32);
    }
}

trax_properties* trax_properties_create() {

    trax_properties* prop = (trax_properties*)malloc(sizeof(trax_properties));

    prop->map = sm_new(32);

    return prop;

}

trax_properties* trax_properties_copy(const trax_properties* original) {

    trax_properties* prop = trax_properties_create();

    copy_properties(original, prop, COPY_ALL);

    return prop;

}

void trax_properties_set(trax_properties* properties, const char* key, const char* value) {

    sm_put(properties->map, key, value);

}

void trax_properties_set_int(trax_properties* properties, const char* key, int value) {

    char tmp[128];
    sprintf(tmp, "%d", value);
    trax_properties_set(properties, key, tmp);

}

void trax_properties_set_float(trax_properties* properties, const char* key, float value) {

    char tmp[128];
    sprintf(tmp, "%f", value);
    trax_properties_set(properties, key, tmp);

}

char* trax_properties_get(const trax_properties* properties, const char* key) {

    char* value;
    int size = sm_get(properties->map, key, NULL, 0);

    if (size < 1) return NULL;

    value = (char *) malloc(sizeof(trax_properties) * size);

    sm_get(properties->map, key, value, size);

    return value;
}

int trax_properties_get_int(const trax_properties* properties, const char* key, int def) {

    char* end;
    long ret;
    char* value = trax_properties_get(properties, key);

    if (value == NULL) return def;

    if (value[0] != '\0') {
        ret = (int) strtod(value, &end);
        ret = (*end == '\0' && end != value) ? ret : def;
    }

    free(value);
    return (int)ret;

}


float trax_properties_get_float(const trax_properties* properties, const char* key, float def) {

    char* end;
    float ret;
    char* value = trax_properties_get(properties, key);

    if (value == NULL) return def;

    if (value[0] != '\0') {
        ret = (float) strtod(value, &end);
        ret = (*end == '\0' && end != value) ? ret : def;
    }

    free(value);
    return ret;

}

int trax_properties_count(const trax_properties* properties) {

    return sm_get_count(properties->map);

}

void trax_properties_enumerate(const trax_properties* properties, trax_enumerator enumerator, const void* object) {
    if (properties && enumerator) {

        sm_enum(properties->map, enumerator, object);
    }
}

trax_image_list* trax_image_list_create() {

    int i;

    trax_image_list* list = (trax_image_list*)malloc(sizeof(trax_image_list));

    for (i = 0; i < TRAX_CHANNELS; i++)
        list->images[i] = NULL;

    return list;
}

void trax_image_list_clear(trax_image_list* list) {

    int i;

    for (i = 0; i < TRAX_CHANNELS; i++) {
        if (!list->images[i]) continue;

        trax_image_release(&list->images[i]);

    }

}

void trax_image_list_release(trax_image_list** list) {

    int i;

    for (i = 0; i < TRAX_CHANNELS; i++)
        (*list)->images[i] = NULL;

    free(*list);

    *list = NULL;
}

trax_image* trax_image_list_get(const trax_image_list* list, int channel) {
    int i = TRAX_CHANNEL_INDEX(channel);
    if (i < 0 || i > TRAX_CHANNELS) return NULL;

    return list->images[i];
}

void trax_image_list_set(trax_image_list* list, trax_image* image, int channel) {
    int i = TRAX_CHANNEL_INDEX(channel);
    if (i < 0 || i > TRAX_CHANNELS) return;

    list->images[i] = image;
}

int trax_image_list_count(int channels) {

    // Count the number of channels
    int count; // argument_count accumulates the total bits set in channels
    for (count = 0; channels; count++) {
        channels &= channels - 1; // clear the least significant bit set
    }

    return count;
}