/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef TRAX_H
#define TRAX_H

#include <stdio.h>
#include <fcntl.h>

#ifdef TRAX_STATIC_DEFINE
#  define __TRAX_EXPORT
#else
#  ifndef __TRAX_EXPORT
#    if defined(_MSC_VER)
#      ifdef trax_EXPORTS
         /* We are building this library */
#        define __TRAX_EXPORT __declspec(dllexport)
#      else
         /* We are using this library */
#        define __TRAX_EXPORT __declspec(dllimport)
#      endif
#    elif defined(__GNUC__)
#      ifdef trax_EXPORTS
         /* We are building this library */
#        define __TRAX_EXPORT __attribute__((visibility("default")))
#      else
         /* We are using this library */
#        define __TRAX_EXPORT __attribute__((visibility("default")))
#      endif
#    endif
#  endif
#endif

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#define TRAX_NO_LOG (~0)
#pragma comment(lib, "ws2_32.lib")
#else
#define TRAX_NO_LOG -1
#endif

#define TRAX_VERSION 1

#define TRAX_ERROR -1
#define TRAX_OK 0
#define TRAX_HELLO 1
#define TRAX_INITIALIZE 2
#define TRAX_FRAME 3
#define TRAX_QUIT 4
#define TRAX_STATE 5

#define TRAX_IMAGE_EMPTY 0
#define TRAX_IMAGE_PATH 1
#define TRAX_IMAGE_URL 2
#define TRAX_IMAGE_MEMORY 4
#define TRAX_IMAGE_BUFFER 8

#define TRAX_IMAGE_ANY (TRAX_IMAGE_PATH | TRAX_IMAGE_URL | TRAX_IMAGE_MEMORY | TRAX_IMAGE_BUFFER)

#define TRAX_IMAGE_BUFFER_ILLEGAL 0
#define TRAX_IMAGE_BUFFER_PNG 1
#define TRAX_IMAGE_BUFFER_JPEG 2

#define TRAX_IMAGE_MEMORY_ILLEGAL 0
#define TRAX_IMAGE_MEMORY_GRAY8 1
#define TRAX_IMAGE_MEMORY_GRAY16 2
#define TRAX_IMAGE_MEMORY_RGB 3

#define TRAX_REGION_EMPTY 0
#define TRAX_REGION_SPECIAL 1
#define TRAX_REGION_RECTANGLE 2
#define TRAX_REGION_POLYGON 4
#define TRAX_REGION_MASK 8 // Not implemented yet!

#define TRAX_REGION_ANY (TRAX_REGION_RECTANGLE | TRAX_REGION_POLYGON)

#define TRAX_FLAG_VALID 1
#define TRAX_FLAG_SERVER 2
#define TRAX_FLAG_TERMINATED 4

#define TRAX_PARAMETER_VERSION 0
#define TRAX_PARAMETER_CLIENT 1
#define TRAX_PARAMETER_SOCKET 2
#define TRAX_PARAMETER_REGION 3
#define TRAX_PARAMETER_IMAGE 4

#define TRAX_LOCALHOST "127.0.0.1"

#define TRAX_SUPPORTS(F, M) ((F & M) != 0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A trax image data structure. Use trax_image_* functions to access the data.
**/
typedef struct trax_image {
    short type;
    int width;
    int height;
    int format;
    char* data;
} trax_image;

/**
 * A placeholder for region structure. Use the trax_region_* functions to manipulate
 * the data.
**/
typedef void trax_region;

/**
 * A placeholder for properties structure. Use the trax_properties_* functions to manipulate
 * the data.
**/
typedef struct trax_properties trax_properties;

typedef struct trax_bounds {

    float top;
    float bottom;
    float left;
    float right;

} trax_bounds;

typedef void(*trax_logger)(const char *string, int length, void *obj);

typedef void(*trax_enumerator)(const char *key, const char *value, const void *obj);

/**
 * Some basic configuration data used to set up the server.
**/
typedef struct trax_logging {
    int flags;
    trax_logger callback;
    void* data;
} trax_logging;

/**
 * Some basic configuration data used to set up the server.
**/
typedef struct trax_configuration {
    int format_region;
    int format_image;
} trax_configuration;

/**
 * Core object of the protocol. Do not manipulate it directly.
**/
typedef struct trax_handle {
    int flags;
    int version;
    void* stream;
    trax_properties* properties;
    trax_logging logging;
    trax_configuration config;
} trax_handle;

__TRAX_EXPORT extern const trax_logging trax_no_log;

__TRAX_EXPORT extern const trax_bounds trax_no_bounds;

/**
 * Returns library version.
**/
__TRAX_EXPORT const char* trax_version();

/**
 * A handy function to initialize a logging configuration structure.
**/
__TRAX_EXPORT trax_logging trax_logger_setup(trax_logger callback, void* data, int flags);

/**
 * A handy function to initialize a logging configuration structure for file logging.
**/
__TRAX_EXPORT trax_logging trax_logger_setup_file(FILE* file);

/**
 * Setups the protocol state object for the client and returns a handle object.
**/
__TRAX_EXPORT trax_handle* trax_client_setup_file(int input, int output, const trax_logging log);

/**
 * Setups the protocol state object for the client and returns a handle object.
**/
__TRAX_EXPORT trax_handle* trax_client_setup_socket(int server, int timeout, const trax_logging log);

/**
 * Waits for a valid protocol message from the server.
**/
__TRAX_EXPORT int trax_client_wait(trax_handle* client, trax_region** region, trax_properties* properties);

/**
 * Sends an initialize message.
**/
__TRAX_EXPORT int trax_client_initialize(trax_handle* client, trax_image* image, trax_region* region, trax_properties* properties);

/**
 * Sends a frame message.
**/
__TRAX_EXPORT int trax_client_frame(trax_handle* client, trax_image* image, trax_properties* properties);

/**
 * Setups the protocol for the server side and returns a handle object.
**/
__TRAX_EXPORT trax_handle* trax_server_setup(trax_configuration config, const trax_logging log);

/**
 * Setups the protocol for the server side and returns a handle object.
**/
__TRAX_EXPORT trax_handle* trax_server_setup_file(trax_configuration config, int input, int output, const trax_logging log);

/**
 * Waits for a valid protocol message from the client.
**/
__TRAX_EXPORT int trax_server_wait(trax_handle* server, trax_image** image, trax_region** region, trax_properties* properties);

/**
 * Sends a status reply to the client.
**/
__TRAX_EXPORT int trax_server_reply(trax_handle* server, trax_region* region, trax_properties* properties);

/**
 * Used in client and server. Closes communication, sends quit message if needed.
 * Releases the handle structure.
**/
__TRAX_EXPORT int trax_cleanup(trax_handle** handle);

/**
 * Sets the parameter of the client or server instance.
**/
__TRAX_EXPORT int trax_set_parameter(trax_handle* handle, int id, int value);

/**
 * Gets the parameter of the client or server instance.
**/
__TRAX_EXPORT int trax_get_parameter(trax_handle* handle, int id, int* value);

/**
 * Releases image structure, frees allocated memory.
**/
__TRAX_EXPORT void trax_image_release(trax_image** image);

/**
 * Creates a file-system path image description.
**/
__TRAX_EXPORT trax_image* trax_image_create_path(const char* path);

/**
 * Creates a URL path image description.
**/
__TRAX_EXPORT trax_image* trax_image_create_url(const char* url);

/**
 * Creates a raw buffer image description.
**/
__TRAX_EXPORT trax_image* trax_image_create_memory(int width, int height, int format);

/**
 * Creates a file buffer image description.
**/
__TRAX_EXPORT trax_image* trax_image_create_buffer(int length, const char* data);

/**
 * Returns a type of the image handle.
**/
__TRAX_EXPORT int trax_image_get_type(const trax_image* image);

/**
 * Returns a file path from a file-system path image description. This function
 * returns a pointer to the internal data which should not be modified.
**/
__TRAX_EXPORT const char* trax_image_get_path(const trax_image* image);

/**
 * Returns a file path from a URL path image description. This function
 * returns a pointer to the internal data which should not be modified.
**/
__TRAX_EXPORT const char* trax_image_get_url(const trax_image* image);

/**
 * Returns the header data of a memory image.
**/
__TRAX_EXPORT void trax_image_get_memory_header(const trax_image* image, int* width, int* height, int* format);

/**
 * Returns a pointer for a writeable row in a data array of an image.
**/
__TRAX_EXPORT char* trax_image_write_memory_row(trax_image* image, int row);

/**
 * Returns a read-only pointer for a row in a data array of an image.
**/
__TRAX_EXPORT const char* trax_image_get_memory_row(const trax_image* image, int row);

/**
 * Returns a file buffer and its length. This function
 * returns a pointer to the internal data which should not be modified.
**/
__TRAX_EXPORT const char* trax_image_get_buffer(const trax_image* image, int* length, int* format);

/**
 * Releases region structure, frees allocated memory.
**/
__TRAX_EXPORT void trax_region_release(trax_region** region);

/**
 * Returns type identifier of the region object.
**/
__TRAX_EXPORT int trax_region_get_type(const trax_region* region);

/**
 * Creates a special region object. Only one paramter (region code) required.
**/
__TRAX_EXPORT trax_region* trax_region_create_special(int code);

/**
 * Sets the code of a special region.
**/
__TRAX_EXPORT void trax_region_set_special(trax_region* region, int code);

/**
 * Returns a code of a special region object.
**/
__TRAX_EXPORT int trax_region_get_special(const trax_region* region);

/**
 * Creates a rectangle region.
**/
__TRAX_EXPORT trax_region* trax_region_create_rectangle(float x, float y, float width, float height);

/**
 * Sets the coordinates for a rectangle region.
**/
__TRAX_EXPORT void trax_region_set_rectangle(trax_region* region, float x, float y, float width, float height);

/**
 * Retreives coordinate from a rectangle region object.
**/
__TRAX_EXPORT void trax_region_get_rectangle(const trax_region* region, float* x, float* y, float* width, float* height);

/**
 * Creates a polygon region object for a given amout of points. Note that the coordinates of the points
 * are arbitrary and have to be set after allocation.
**/
__TRAX_EXPORT trax_region* trax_region_create_polygon(int count);

/**
 * Sets coordinates of a given point in the polygon.
**/
__TRAX_EXPORT void trax_region_set_polygon_point(trax_region* region, int index, float x, float y);

/**
 * Retrieves the coordinates of a specific point in the polygon.
**/
__TRAX_EXPORT void trax_region_get_polygon_point(const trax_region* region, int index, float* x, float* y);

/**
 * Returns the number of points in the polygon.
**/
__TRAX_EXPORT int trax_region_get_polygon_count(const trax_region* region);

/**
 * Calculates a bounding box region that bounds the input region.
 **/
__TRAX_EXPORT trax_bounds trax_region_bounds(const trax_region* region);

/**
 * Calculates if the region contains a given point.
 **/
__TRAX_EXPORT int trax_region_contains(const trax_region* region, float x, float y);

/**
 * Clones a region object.
 **/
__TRAX_EXPORT trax_region* trax_region_clone(const trax_region* region);

/**
 * Converts region between different formats (if possible).
 **/
__TRAX_EXPORT trax_region* trax_region_convert(const trax_region* region, int format);

/**
 * Calculates the spatial Jaccard index for two regions (overlap).
 **/
__TRAX_EXPORT float trax_region_overlap(const trax_region* a, const trax_region* b, const trax_bounds bounds);

/**
 * Encodes a region object to a string representation.
 **/
__TRAX_EXPORT char* trax_region_encode(const trax_region* region);

/**
 * Decodes string representation of a region to an object.
 **/
__TRAX_EXPORT trax_region* trax_region_decode(const char* data);

/**
 * Destroy a properties object and clean up the memory.
 **/
__TRAX_EXPORT void trax_properties_release(trax_properties** properties);

/**
 * Clear a properties object.
 **/
__TRAX_EXPORT void trax_properties_clear(trax_properties* properties);

/**
 * Create a property object.
 **/
__TRAX_EXPORT trax_properties* trax_properties_create();

/**
 * Set a string property (the value string is cloned).
 **/
__TRAX_EXPORT void trax_properties_set(trax_properties* properties, const char* key, const char* value);

/**
 * Set an integer property. The value will be encoded as a string.
 **/
__TRAX_EXPORT void trax_properties_set_int(trax_properties* properties, const char* key, int value);

/**
 * Set an floating point value property. The value will be encoded as a string.
 **/
__TRAX_EXPORT void trax_properties_set_float(trax_properties* properties, const char* key, float value);

/**
 * Get a string property. The resulting string is a clone of the one stored so it should
 * be released when not needed anymore.
 **/
__TRAX_EXPORT char* trax_properties_get(const trax_properties* properties, const char* key);

/**
 * Get an integer property. A stored string value is converted to an integer. If this is not possible
 * or the property does not exist a given default value is returned.
 **/
__TRAX_EXPORT int trax_properties_get_int(const trax_properties* properties, const char* key, int def);

/**
 * Get an floating point value property. A stored string value is converted to an integer. If this is not possible
 * or the property does not exist a given default value is returned.
 **/
__TRAX_EXPORT float trax_properties_get_float(const trax_properties* properties, const char* key, float def);

/**
 * Iterate over the property set using a callback function. An optional pointer can be given and is forwarded
 * to the callback.
 **/
__TRAX_EXPORT void trax_properties_enumerate(trax_properties* properties, trax_enumerator enumerator, const void* object);

#ifdef __cplusplus
}

#include <string>
#include <map>
#include <algorithm>
#include <iostream>

namespace trax {

class Image;
class Region;
class Properties;

typedef trax_enumerator Enumerator;

class __TRAX_EXPORT Configuration : public ::trax_configuration {
public:
    Configuration(trax_configuration config);
    Configuration(int image_formats, int region_formats);
    virtual ~Configuration();
};

class __TRAX_EXPORT Logging : public ::trax_logging {
public:
    Logging(trax_logging logging);
    Logging(trax_logger callback = NULL, void* data = NULL, int flags = 0);
    virtual ~Logging();
};

class __TRAX_EXPORT Bounds : public ::trax_bounds {
public:
    Bounds();
    Bounds(trax_bounds bounds);
    Bounds(float left, float top, float right, float bottom);
    virtual ~Bounds();
};

class __TRAX_EXPORT Wrapper {
public:
    virtual ~Wrapper();

protected:
    Wrapper();
    
    Wrapper(const Wrapper& count);

    void swap(Wrapper& lhs) throw();

    long claims() throw();

    /**
     * Call after the wrapped pointer has been created or copied to increase
     * reference count;
    **/
    void acquire();

    /**
     * Call instead of releasing memory to decrease reference count. If the 
     * reference count comes to zero then cleanup() is called.
    **/
    void release() throw();

    virtual void cleanup() = 0;

private:

    long* pn;

};

class __TRAX_EXPORT Handle: public Wrapper {
public:
    /**
     * Closes communication, sends quit message if needed.
    **/
    virtual ~Handle();

    /**
     * Sets the parameter for the client or server instance.
    **/
    int set_parameter(int id, int value);

    /**
     * Gets the parameter for the client or server instance.
    **/
    int get_parameter(int id, int* value);

protected:

    virtual void cleanup();

    void wrap(trax_handle* obj);

    Handle();
    
    Handle(const Handle& original);
    
    trax_handle* handle;
};

class __TRAX_EXPORT Client: public Handle {
public:
    /**
     * Sets up the protocol for the client side and returns a handle object.
    **/
    Client(int input, int output, Logging logger);

    /**
     * Sets up the protocol for the client side and returns a handle object.
    **/
    Client(int server, Logging logger,  int timeout = -1);

    virtual ~Client();

    /**
    * Waits for a valid protocol message from the server.
    **/
    int wait(Region& region, Properties& properties);

    /**
    * Sends an initialize message.
    **/
    int initialize(const Image& image, const Region& region, const Properties& properties);

    /**
    * Sends a frame message.
    **/
    int frame(const Image& image, const Properties& properties);

    const Configuration configuration();

protected:

    using Handle::cleanup;

private:

    Client& operator=(Client p) throw();

};

class __TRAX_EXPORT Server: public Handle {
public:

    /**
     * Sets up the protocol for the server side and returns a handle object.
    **/
    Server(Configuration configuration, Logging log);

    virtual ~Server();

    /**
     * Waits for a valid protocol message from the client.
    **/
    int wait(Image& image, Region& region, Properties& properties);

    /**
     * Sends a status reply to the client.
    **/
    int reply(const Region& region, const Properties& properties);

    const Configuration configuration();

private:
    Server& operator=(Server p) throw();

};

class __TRAX_EXPORT Image : public Wrapper {
friend class Client;
friend class Server;
public:

    Image();

    Image(const Image& original);

    /**
     * Creates a file-system path image description.
    **/
    static Image create_path(const std::string& path);

    /**
     * Creates a URL path image description.
    **/
    static Image create_url(const std::string& url);

    /**
     * Creates a raw buffer image description.
    **/
    static Image create_memory(int width, int height, int format);

    /**
     * Creates a file buffer image description.
    **/
    static Image create_buffer(int length, const char* data);

    /**
     * Releases image structure, frees allocated memory.
    **/
    virtual ~Image();

    /**
     * Returns a type of the image handle.
    **/
    int type() const;

    /**
     * Checks if image container is empty.
    **/
    bool empty() const;

    /**
     * Returns a file path from a file-system path image description. This function
     * returns a pointer to the internal data which should not be modified.
    **/
    const std::string get_path() const;

    /**
     * Returns a file path from a URL path image description. This function
     * returns a pointer to the internal data which should not be modified.
    **/
    const std::string get_url() const;

    /**
     * Returns the header data of a memory image.
    **/
    void get_memory_header(int* width, int* height, int* format) const;

    /**
     * Returns a pointer for a writeable row in a data array of an image.
    **/
    char* write_memory_row(int row);

    /**
     * Returns a read-only pointer for a row in a data array of an image.
    **/
    const char* get_memory_row(int row) const;

    /**
     * Returns a file buffer and its length. This function
     * returns a pointer to the internal data which should not be modified.
    **/
    const char* get_buffer(int* length, int* format) const;

    Image& operator=(Image lhs) throw();

protected:

    virtual void cleanup();

    void wrap(trax_image* obj);

private:

    trax_image* image;

};

class __TRAX_EXPORT Region : public Wrapper {
friend class Client;
friend class Server;
public:

    Region();

    Region(const Region& original);

    /**
     * Creates a special region object. Only one paramter (region code) required.
    **/
    static Region create_special(int code);

    /**
     * Creates a rectangle region.
    **/
    static Region create_rectangle(float x, float y, float width, float height);

    /**
     * Creates a polygon region object for a given amout of points. Note that the coordinates of the points
     * are arbitrary and have to be set after allocation.
    **/
    static Region create_polygon(int count);

    /**
     * Releases region, frees allocated memory.
    **/
    virtual ~Region();

    /**
     * Returns type identifier of the region object.
    **/
    int type() const;

    /**
     * Checks if region container is empty.
    **/
    bool empty() const;

    /**
     * Sets the code of a special region.
    **/
    void set(int code);

    /**
     * Returns a code of a special region object.
    **/
    int get() const;

    /**
     * Sets the coordinates for a rectangle region.
    **/
    void set(float x, float y, float width, float height);

    /**
     * Retreives coordinate from a rectangle region object.
    **/
    void get(float* x, float* y, float* width, float* height) const;

    /**
     * Sets coordinates of a given point in the polygon.
    **/
    void set_polygon_point(int index, float x, float y);

    /**
     * Retrieves the coordinates of a specific point in the polygon.
    **/
    void get_polygon_point(int index, float* x, float* y) const;

    /**
     * Returns the number of points in the polygon.
    **/
    int get_polygon_count() const;

    /**
     * Computes bounds of a region.
     **/
    Bounds bounds() const;

    bool contains(float x, float y) const;

    Region convert(int type) const;

    float overlap(const Region& region, const Bounds& bounds = Bounds()) const;

    Region& operator=(Region lhs) throw();

    operator std::string () const;

    friend __TRAX_EXPORT std::ostream& operator<< (std::ostream& output, const Region& region);

    friend __TRAX_EXPORT std::istream& operator>> (std::istream& input, Region &D);

protected:

    virtual void cleanup();

    void wrap(trax_region* obj);

private:

    trax_region* region;
};

class __TRAX_EXPORT Properties : public Wrapper {
friend class Client;
friend class Server;
public:

    /**
     * Create a property object.
     **/
    Properties();

    /**
     * A copy constructor.
     **/
    Properties(const Properties& original);

    /**
     * Destroy a properties object and clean up the memory.
     **/
    virtual ~Properties();

    /**
     * Clear a properties object.
     **/
    void clear();

    /**
     * Set a string property (the value string is cloned).
     **/
    void set(const std::string key, const std::string value);

    /**
     * Set an integer property. The value will be encoded as a string.
     **/
    void set(const std::string key, int value);

    /**
     * Set an floating point value property. The value will be encoded as a string.
     **/
    void set(const std::string key, float value);

    /**
     * Get a string property.
     **/
    std::string get(const std::string key);

    /**
     * Get an integer property. A stored string value is converted to an integer. If this is not possible
     * or the property does not exist a given default value is returned.
     **/
    int get(const std::string key, int def);

    /**
     * Get an floating point value property. A stored string value is converted to an float. If this is not possible
     * or the property does not exist a given default value is returned.
     **/
    float get(const std::string key, float def);

    /**
     * Get an boolean point value property. A stored string value is converted to an integer and checked if it is zero. If this is not possible
     * or the property does not exist a given default value is returned.
     **/
    bool get(const std::string key, bool def);

    /**
     * Iterate over the property set using a callback function. An optional pointer can be given and is forwarded
     * to the callback.
     **/
    void enumerate(Enumerator enumerator, void* object);

    void from_map(const std::map<std::string, std::string>& m);

    void to_map(std::map<std::string, std::string>& m) const;

    Properties& operator=(Properties lhs) throw();

    friend __TRAX_EXPORT std::ostream& operator<< (std::ostream& output, const Properties& properties);
    
protected:

    virtual void cleanup();

    void wrap(trax_properties* obj);

private:

    void ensure_unique();

    trax_properties* properties;

};


}

#endif

#endif
