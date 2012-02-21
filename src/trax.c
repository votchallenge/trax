/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <unistd.h>
#include <string.h>
#include "trax.h"
#include "strmap.h"

// TODO: arbitrary line length
// TODO: tokenizer

#define _HELPER(x) #x
#define __TOKEN_INIT TRAX_PREFIX "initialize"
#define __TOKEN_HELLO TRAX_PREFIX "hello"
#define __TOKEN_SELECT TRAX_PREFIX "select"
#define __TOKEN_FRAME TRAX_PREFIX "frame"
#define __TOKEN_QUIT TRAX_PREFIX "quit"
#define __TOKEN_POSITION TRAX_PREFIX "position"

#define LOG(MSG) if (__log && (flags & TRAX_LOG_DEBUG)) { fprintf(__log, "%s\n", MSG); fflush(__log); }
#define BUFFER_LENGTH 10

FILE* __log = 0;
int flags = 0;

struct trax_properties {
    StrMap *map;
};

void __output(const char *message) {

    fputs(message, stdout);
    if (__log && (flags & TRAX_LOG_OUTPUT))
        fputs(message, __log);

}

void __output_enum(const char *key, const char *value, const void *obj) {
    
    fprintf(stdout, " %s=%s", key, value);
    if (__log && (flags & TRAX_LOG_OUTPUT))
        fprintf(__log, " %s=%s", key, value);
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
        if(c == EOF)
            break;

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

#define _FLAG_STRING 1

int __next_token(char* str, int start, char* buffer, int len, int flags) 
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


void trax_test() 
{
// init 10 10 50 50 "/home/lukacu/111.png" aa=bla bb=c
    trax_rectangle rectangle;
    trax_imagepath path;
    trax_properties* prop = trax_properties_create();

    trax_wait(path, prop, &rectangle);

    sm_enum(prop->map, __output_enum, NULL);
    trax_properties_release(&prop);
}

int trax_setup(const char* log, int flg) {

    char message[1024];
    int pos = 0;
    char* line = NULL;
    int size = 0;
    char* buffer;

    flags = flg;

    if (log)
        __log = fopen(log, "w");
    else
        __log = NULL;

    LOG("LOG START");

    // TODO: this is only a dummy message
    sprintf(message,"%s in.boundingbox out.boundingbox source.path\n", __TOKEN_HELLO);
    __output(message);

    LOG("SELECT wait");

    line = __read_line(stdin);
    size = strlen(line);
    buffer = (char *) malloc(size * sizeof(char));

    if (!line) return TRAX_ERROR;

    if ((pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) < 0) 
    {
        free(line);
        free(buffer);
        return TRAX_ERROR;
    }

    if (strcmp(buffer, __TOKEN_SELECT) == 0) 
    {
        free(line);
        free(buffer);
        return 0;
    }

    free(line);
    free(buffer);
    return TRAX_ERROR;
}

int trax_cleanup() {

    if (__log) {
        LOG("LOG END");
        fclose(__log);
        __log = 0;
    }


    return 0;
}

int trax_wait(trax_imagepath path, trax_properties* properties, trax_rectangle * rectangle) 
{
    LOG("FRAME wait");

    int pos = 0;
    char* line = __read_line(stdin);
    int size = strlen(line);
    char buffer[size];
    int result = TRAX_ERROR;

    if (!line) return TRAX_ERROR;

    if (__log && (flags & TRAX_LOG_INPUT)) {
        fputs(line, __log);
        fputs("\n", __log);
        fflush(__log);
    }

    if ((pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) < 0) {
        free(line);

        return TRAX_ERROR;
    }

    if (strcmp(buffer, __TOKEN_FRAME) == 0) 
    {
        LOG("FRAME rcv");

        result = TRAX_FRAME;

        if ((pos = __next_token(line, pos, path, TRAX_PATH_MAX_LENGTH, _FLAG_STRING)) < 0) 
        {
            free(line);
            return result;
        }

        if (properties) {
            while ( (pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) > 0) 
            {
                
                char* pos = strstr(buffer, "=");

                if (pos > 0) {
                    const char* key = buffer;
                    *pos = '\0';
                    char* value = &(pos[1]);
                    trax_properties_set(properties, key, value);
                }

            }
        }

        LOG("FRAME ok");

        free(line);
        return result;
    } else if (strcmp(buffer, __TOKEN_QUIT) == 0) {
        free(line);
        LOG("QUIT rcv");
        return TRAX_QUIT;
    } else if (strcmp(buffer, __TOKEN_INIT) == 0) {

        LOG("INIT rcv");
        result = TRAX_INIT;

        if ((pos = __next_token(line, pos, buffer, size, 0)) > 0) 
        {
            rectangle->x = (float) atof(buffer);
        } else {
            result = TRAX_ERROR; goto end;}

        if ((pos = __next_token(line, pos, buffer, size, 0)) > 0) 
        {
            rectangle->y = (float) atof(buffer);
        } else {result = TRAX_ERROR; goto end;}

        if ((pos = __next_token(line, pos, buffer, size, 0)) > 0) 
        {
            rectangle->width = (float) atof(buffer);
        } else {result = TRAX_ERROR; goto end;}

        if ((pos = __next_token(line, pos, buffer, size, 0)) > 0) 
        {
            rectangle->height = (float) atof(buffer);
        } else {result = TRAX_ERROR; goto end;}

        if ((pos = __next_token(line, pos, path, TRAX_PATH_MAX_LENGTH, _FLAG_STRING)) < 0) 
        {
            result = TRAX_ERROR; goto end;
        }

        if (properties) {
            while ( (pos = __next_token(line, pos, buffer, size, _FLAG_STRING)) > 0) 
            {
                
                char* pos = strstr(buffer, "=");

                if (pos > 0) {
                    const char* key = buffer;
                    *pos = '\0';
                    char* value = &(pos[1]);
                    trax_properties_set(properties, key, value);
                }

            }
        }

        LOG("INIT ok");

end:

        free(line);
        return result;

    }

    free(line);

    return TRAX_ERROR;
}

void trax_report_position(trax_rectangle position, trax_properties* properties) {

    char message[1024];

    sprintf(message,"%s %f %f %f %f", __TOKEN_POSITION, position.x, position.y,  position.width,  position.height);

    __output(message);

    if (properties) {
        sm_enum(properties->map, __output_enum, NULL);
    }

    sprintf(message, "\n");
    __output(message);

//    fprintf(stdout,"\n");
    fflush(stdout);

    LOG("POSITION snd");

}

void trax_log(const char *message) {

    LOG(message);

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

    char* value = malloc(sizeof(trax_properties) * size);

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

