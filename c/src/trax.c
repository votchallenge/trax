/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "trax.h"
#include "strmap.h"

// TODO: arbitrary line length

#define _HELPER(x) #x
#define __TOKEN_INIT TRAX_PREFIX "initialize"
#define __TOKEN_HELLO TRAX_PREFIX "hello"
#define __TOKEN_FRAME TRAX_PREFIX "frame"
#define __TOKEN_QUIT TRAX_PREFIX "quit"
#define __TOKEN_STATUS TRAX_PREFIX "status"

#define VALIDATE_HANDLE(H) assert((H->flags & TRAX_FLAG_VALID) != 0)
#define VALIDATE_SERVER_HANDLE(H) assert((H->flags & TRAX_FLAG_VALID) && (H->flags & TRAX_FLAG_SERVER))
#define VALIDATE_CLIENT_HANDLE(H) assert((H->flags & TRAX_FLAG_VALID) && !(H->flags & TRAX_FLAG_SERVER))

#define LOG(H, MSG) if (H->log && (H->flags & TRAX_FLAG_LOG_DEBUG)) { fprintf(H->log, "%s\n", MSG); fflush(H->log); }
#define LOG_IN(H, MSG) if (H->log && (H->flags & TRAX_FLAG_LOG_INPUT)) { fprintf(H->log, ">> %s >>\n", MSG); fflush(H->log); }
#define LOG_OUT(H, MSG) if (H->log && (H->flags & TRAX_FLAG_LOG_OUTPUT)) { fprintf(H->log, "<< %s <<\n", MSG); fflush(H->log); }
#define BUFFER_LENGTH 64

struct trax_properties {
    StrMap *map;
};

struct trax_handle {
    int flags;
    int version;
    FILE* log;
    trax_configuration config;
    FILE* input;
    FILE* output;
};

int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void __output(trax_handle* handle, const char *message) {

    fputs(message, handle->output);
    LOG_OUT(handle, message);
    fflush(handle->output);

}

void __output_enum(const char *key, const char *value, const void *obj) {
    
    const trax_handle* handle = (trax_handle *) obj;

    fprintf(handle->output, " %s=%s", key, value);
    if (handle->log && (handle->flags & TRAX_FLAG_LOG_OUTPUT))
        fprintf(handle->log, " %s=%s", key, value);
}

char* __read_line(FILE* file)
{

    char * line = malloc(BUFFER_LENGTH), * linep = line;
    size_t lenmax = BUFFER_LENGTH, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF) {
            if (len == lenmax) {
                free(line);
                return NULL;
            }
            break;
        }

        if(--len == 0) {
            len += BUFFER_LENGTH;
            lenmax += BUFFER_LENGTH;
            char * linen = realloc(linep, lenmax);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if (c == '\n') break;

        *line++ = c;
    }
    *line = '\0';
    return linep;
}

char* __read_protocol_line(FILE* file)
{

    char* line = NULL;

    for (;;) {
        line = __read_line(file);

        if (!line) return NULL;

        if (starts_with(TRAX_PREFIX, line))
            return line;
    }
}

#define _FLAG_STRING 1

int __next_token(const char* str, int start, char* buffer, int len, int flags) 
{
    int i;
    int s = -1;
    int e = -1;
    short quotes = 0;

    for (i = start; str[i] != '\0' ; i++) 
    {
        if (s < 0 && str[i] != ' ')
        {
            if ((flags & _FLAG_STRING != 0) && str[i] == '"') 
            {
                quotes = 1;
                s = i+1;
            }
            else s = i;
            continue;
        }
        if (s >= 0 && ((!quotes && str[i] == ' ') || (quotes && str[i] == '"' && (i == start || str[i-1] != '\''))))
        {
            e = i;
            break;
        }
    }

    buffer[0] = '\0';

    if (s < 0) return -1;

    if (e < 0 && !quotes) e = i;
    if (e < 0 && quotes) return -1;

    if (e - s > len - 1) return -1;


    memcpy(buffer, &(str[s]), e - s);

    buffer[e - s] = '\0';

    return str[e] == '\0' ? e : (quotes ? e+2 : e+1);
}

int __parse_properties(const char* line, int* pos, int size, trax_properties* properties) {

    char buffer[size];

    while ( (*pos = __next_token(line, *pos, buffer, size, _FLAG_STRING)) > 0) 
    {
        
        char* split = strstr(buffer, "=");

        if (split > 0) {
            const char* key = buffer;
            *split = '\0';
            char* value = &(split[1]);
            trax_properties_set(properties, key, value);
        }

    }

    return 1;
}

int __parse_rectangle(const char* line, int* pos, int size, trax_rectangle* rectangle) {

    char buffer[size];

    if ((*pos = __next_token(line, *pos, buffer, size, 0)) > 0) 
    {
        rectangle->x = (float) atof(buffer);
    } else { return 0; }

    if ((*pos = __next_token(line, *pos, buffer, size, 0)) > 0) 
    {
        rectangle->y = (float) atof(buffer);
    } else { return 0; }

    if ((*pos = __next_token(line, *pos, buffer, size, 0)) > 0) 
    {
        rectangle->width = (float) atof(buffer);
    } else { return 0; }

    if ((*pos = __next_token(line, *pos, buffer, size, 0)) > 0) 
    {
        rectangle->height = (float) atof(buffer);
    } else { return 0; }

    return 1;
}

void trax_test() 
{
// init 10 10 50 50 "/home/lukacu/111.png" aa=bla bb=c
    trax_rectangle rectangle;
    trax_imagepath path;
    trax_properties* prop = trax_properties_create();

    trax_handle handle;

    trax_wait(path, prop, &rectangle);

    sm_enum(prop->map, __output_enum, &handle);
    trax_properties_release(&prop);
}

trax_handle* trax_client_setup(FILE* input, FILE* output, FILE* log, int flags) {

    char message[1024];
    int pos = 0;
    char* line = NULL;
    int size = 0;
    char* buffer;
    trax_properties* config;

    trax_handle* client = malloc(sizeof(trax_handle));


    client->flags = (flags & !TRAX_FLAG_SERVER) | TRAX_FLAG_VALID;

    if (log)
        client->log = log;
    else
        client->log = NULL;

    client->input = input;
    client->output = output;

    LOG(client, "LOG START");

    LOG(client, "HELLO wait");

    line = __read_protocol_line(client->input);
    if (!line) return TRAX_ERROR;
    size = strlen(line);
    buffer = (char *) malloc(size * sizeof(char));

    LOG_IN(client, line);

    if ((pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) < 0) 
    {
        free(line);
        free(buffer);
        return TRAX_ERROR;
    }

    if (strcmp(buffer, __TOKEN_HELLO) == 0) 
    {

        config = trax_properties_create();

        __parse_properties(line, &pos, size, config);

        client->config.format_region = TRAX_REGION_RECTANGLE;
        client->config.format_image = TRAX_IMAGE_PATH;
        client->version = trax_properties_get_int(config, "trax.version", 1);

        trax_properties_release(&config);

        free(line);
        free(buffer);

        LOG(client, "HELLO rcv");
        return client;
    }

    free(line);
    free(buffer);
    free(client);
    return NULL;

}

int trax_client_wait(trax_handle* client, trax_properties* properties, trax_region* region) {

    char* line;
    char* buffer;
    int size, pos;
    int result = TRAX_ERROR;

    VERIFY_CLIENT_HANDLE(client);

    pos = 0;
    line = __read_protocol_line(client->input);
    size = strlen(line);
    buffer = (char*) malloc(sizeof(char) * size);

    LOG(client, "STATUS wait");

    if (!line) {
        free(buffer);
        return TRAX_ERROR;

    }

    LOG_IN(client, line);

    if ((pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) < 0) {
        free(line);
        free(buffer);
        return TRAX_ERROR;
    }

    if (strcmp(buffer, __TOKEN_STATUS) == 0) 
    {
        LOG(client, "STATUS rcv");

        result = TRAX_STATUS;

        switch (client->config.format_region) {
        case TRAX_REGION_RECTANGLE:
            if (!__parse_rectangle(line, &pos, size, &(region->data.rectangle)))
                {result = TRAX_ERROR; goto end;}
            region->type = TRAX_REGION_RECTANGLE;
            break;
        default:
            result = TRAX_ERROR; goto end;
        }

        if (properties) 
            __parse_properties(line, &pos, size, properties);        

        LOG(client, "STATUS ok");

end:

        free(buffer);
        free(line);
        return result;
    } else if (strcmp(buffer, __TOKEN_QUIT) == 0) {

        if (properties) 
            __parse_properties(line, &pos, size, properties);

        free(buffer);
        free(line);
        LOG(client, "QUIT rcv");
        return TRAX_QUIT;
    } 

    free(buffer);
    free(line);
    return TRAX_ERROR; 

}

void trax_client_initialize(trax_handle* client, trax_image image, trax_region region, trax_properties* properties) {

    char message[1024];

    VERIFY_CLIENT_HANDLE(client);

    assert(client->config.format_region == region.type && client->config.format_image == region.type);

    sprintf(message, "%s ", __TOKEN_INIT);
    __output(client, message);

    if (image.type == TRAX_IMAGE_PATH) {
        __output(client, image.data.path);
    } else return;

    if (region.type == TRAX_REGION_RECTANGLE) {
        trax_rectangle position = region.data.rectangle;
        sprintf(message,"%f %f %f %f", position.x, position.y,  position.width,  position.height);
    } else return;
    
    __output(client, message);

    if (properties) {
        sm_enum(properties->map, __output_enum, client);
    }

    sprintf(message, "\n");
    __output(client, message);

    fflush(client->output);

    LOG(client, "INIT snd");

}

void trax_client_frame(trax_handle* client, trax_image image, trax_properties* properties) {

    char message[1024];

    VERIFY_CLIENT_HANDLE(client);

    assert(client->config.format_image == image.type);

    sprintf(message, "%s ", __TOKEN_FRAME);
    __output(client, message);

    if (image.type == TRAX_IMAGE_PATH) {
        __output(client, image.data.path);
    } else return;

    if (properties) {
        sm_enum(properties->map, __output_enum, client);
    }

    sprintf(message, "\n");
    __output(client, message);

    fflush(client->output);

    LOG(client, "FRAME snd");

}

trax_handle* trax_server_setup(trax_configuration config, FILE* log, int flags) {

    char message[1024];
    int pos = 0;
    char* line = NULL;
    int size = 0;
    char* buffer;
    trax_properties* properties;
    trax_handle* server = malloc(sizeof(trax_handle));

    server->flags = (flags | TRAX_FLAG_SERVER) | TRAX_FLAG_VALID;

    if (log)
        server->log = log;
    else
        server->log = NULL;

    server->input = stdin;
    server->output = stdout;

    server->config = config;

    LOG(server, "LOG START");

    sprintf(message,"%s", __TOKEN_HELLO);
    __output(server, message);

    properties = trax_properties_create();

    trax_properties_set_int(properties, "trax.version", TRAX_VERSION);
    trax_properties_set(properties, "trax.region", "rectangle");
    trax_properties_set(properties, "trax.image", "path");

    sm_enum(properties->map, __output_enum, server);

    trax_properties_release(&properties);

    sprintf(message, "\n");
    __output(server, message);

    free(line);
    free(buffer);

    return server;

}

int trax_server_wait(trax_handle* server, trax_image* image, trax_properties* properties, trax_region * region) 
{

    VERIFY_SERVER_HANDLE(server);

    LOG(server, "FRAME wait");

    int pos = 0;
    char* line = __read_protocol_line(server->input);
    int size = strlen(line);
    char buffer[size];
    int result = TRAX_ERROR;

    if (!line) return TRAX_ERROR;

    LOG_IN(server, line);

    if ((pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) < 0) {
        free(line);

        return TRAX_ERROR;
    }

    if (strcmp(buffer, __TOKEN_FRAME) == 0) 
    {
        LOG(server, "FRAME rcv");

        result = TRAX_FRAME;

        switch (server->config.format_region) {
        case TRAX_IMAGE_PATH:
            if ((pos = __next_token(line, pos, image->data.path, TRAX_PATH_MAX_LENGTH, _FLAG_STRING)) < 0) 
                { result = TRAX_ERROR; goto end; }
            image->type = TRAX_IMAGE_PATH;
            break;
        default:
            result = TRAX_ERROR; goto end;
        }

        if (properties) 
            pos = __parse_properties(line, &pos, size, properties);        

        LOG(server, "FRAME ok");

        free(line);
        return result;
    } else if (strcmp(buffer, __TOKEN_QUIT) == 0) {

        if (properties) 
            pos = __parse_properties(line, &pos, size, properties);        

        free(line);
        LOG(server, "QUIT rcv");
        return TRAX_QUIT;
    } else if (strcmp(buffer, __TOKEN_INIT) == 0) {

        LOG(server, "INIT rcv");
        result = TRAX_INIT;

        switch (server->config.format_region) {
        case TRAX_IMAGE_PATH:
            if ((pos = __next_token(line, pos, image->data.path, TRAX_PATH_MAX_LENGTH, _FLAG_STRING)) < 0) 
                { result = TRAX_ERROR; goto end; }
            image->type = TRAX_IMAGE_PATH;
            break;
        default:
            result = TRAX_ERROR; goto end;
        }

        switch (server->config.format_region) {
        case TRAX_REGION_RECTANGLE:
            if (!__parse_rectangle(line, &pos, size, &(region->data.rectangle)))
                {result = TRAX_ERROR; goto end;}
            region->type = TRAX_REGION_RECTANGLE;
            break;
        default:
            result = TRAX_ERROR; goto end;
        }

        if (properties) {
            __parse_properties(line, &pos, size, properties);
        }

        LOG(server, "INIT ok");

end:

        free(line);
        return result;

    }

    free(line);

    return TRAX_ERROR;
}

void trax_server_reply(trax_handle* server, trax_region region, trax_properties* properties) {

    char message[1024];

    VERIFY_SERVER_HANDLE(server);

    assert(server->config.format_region == region.type);

    if (region.type == TRAX_REGION_RECTANGLE) {
        trax_rectangle position = region.data.rectangle;
        sprintf(message,"%s %f %f %f %f", __TOKEN_STATUS, position.x, position.y,  position.width,  position.height);
    } else return;
    
    __output(server, message);

    if (properties) {
        sm_enum(properties->map, __output_enum, server);
    }

    sprintf(message, "\n");
    __output(server, message);

    fflush(server->output);

    LOG(server, "STATUS snd");

}

int trax_cleanup(trax_handle** handle) {

    VERIFY_HANDLE(*handle);

// TODO: send QUIT if client

    if ((*handle)->log) {
        LOG((*handle), "LOG END");
        (*handle)->log = 0;
    }


    free(*handle);
    *handle = 0;

    return 0;
}

void trax_log(trax_handle* handle, const char *message) {

    VERIFY_HANDLE(handle);

    LOG(handle, message);

}

void trax_properties_release(trax_properties** properties) {
    
    if (properties && *properties) {
        if ((*properties)->map) sm_delete((*properties)->map);
        *properties = 0;
    }
}

void trax_properties_clear(trax_properties* properties) {
    
    if (properties) {
        if (properties->map) 
            sm_delete(properties->map);
        properties->map = sm_new(100);
    }
}

trax_properties* trax_properties_create() {

    trax_properties* prop = malloc(sizeof(trax_properties));

    prop->map = sm_new(100);

    return prop;

}

void trax_properties_set(trax_properties* properties, const char* key, char* value) {

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

char* trax_properties_get(trax_properties* properties, const char* key) {

    int size = sm_get(properties->map, key, NULL, 0);

    if (size < 1) return NULL;

    char* value = (char *) malloc(sizeof(trax_properties) * size);

    sm_get(properties->map, key, value, size);

    return value;
}

int trax_properties_get_int(trax_properties* properties, const char* key, int def) {

    char* end;
    long ret;
    char* value = trax_properties_get(properties, key);

    if (value == NULL) return def;

    if (value[0]!='\0') {
        ret = strtol(value,&end, 10);
        ret = (*end=='\0' && end!=value) ? ret : def;
    }

    free(value);
    return (int)ret;

}


float trax_properties_get_float(trax_properties* properties, const char* key, float def) {

    char* end;
    float ret;
    char* value = trax_properties_get(properties, key);

    if (value == NULL) return def;

    if (value[0]!='\0') {
        ret = strtod(value,&end);
        ret = (*end=='\0' && end!=value) ? ret : def;
    }

    free(value);
    return ret;

}

void trax_properties_enumerate(trax_properties* properties, trax_enumerator enumerator, void* object) {
    if (properties && enumerator) {
        
        sm_enum(properties->map, enumerator, object);
    }
}

