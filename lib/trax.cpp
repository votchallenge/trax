
#include "trax.h"

namespace trax {

Wrapper::Wrapper() : pn(NULL) {

}
    
Wrapper::Wrapper(const Wrapper& count) : pn(count.pn) {

}

Wrapper::~Wrapper() {
	
}

void Wrapper::swap(Wrapper& lhs) throw() // never throws
{
    std::swap(pn, lhs.pn);
}

long Wrapper::claims() throw() {
	long count = 0;
	if (NULL != pn)
	{
	    count = *pn;
	}
	return count;
}

void Wrapper::acquire() {
    if (NULL == pn)
        pn = new long(1); // may throw std::bad_alloc
    else
        ++(*pn);
}

void Wrapper::release() throw() {
    if (NULL != pn) {
        --(*pn);
        if (0 == *pn) {
            cleanup();
            delete pn;
        }
        pn = NULL;
    }
}

Handle::Handle() {
	handle = NULL;
}

Handle::~Handle() {
	release();
}

void Handle::cleanup() {
	trax_cleanup(&handle);
}

int Handle::set_parameter(int id, int value) {
	return trax_set_parameter(handle, id, value);
}

int Handle::get_parameter(int id, int* value) {
	return trax_get_parameter(handle, id, value);
}

void Handle::wrap(trax_handle* obj) {
	release();
	handle = obj;
	acquire();
}

Client::Client(int input, int output, trax_logger log) {
	wrap(trax_client_setup_file(input, output,log));
}

Client::Client(int server, trax_logger log, int timeout) {
	wrap(trax_client_setup_socket(server, timeout, log));
}

Client::~Client() {
	// Cleanup done in Handle
}

int Client::wait(Region& region, Properties& properties) {

	trax_region* tregion = NULL;
	int result = trax_client_wait(handle, &tregion, properties.properties);

	if (tregion)
		region.wrap(tregion);

	return result;

}

int Client::initialize(const Image& image, const Region& region, const Properties& properties) {

	return trax_client_initialize(handle, image.image, region.region, properties.properties);

}

int Client::frame(const Image& image, const Properties& properties) {

	return trax_client_frame(handle, image.image, properties.properties);

}

const Configuration Client::configuration() {

	return handle->config;

}

Server::Server(Configuration configuration, trax_logger log) {

	wrap(trax_server_setup(configuration, log));

}

Server::~Server() {
	// Cleanup done in Handle
}

int Server::wait(Image& image, Region& region, Properties& properties) {

	trax_image* timage = NULL;
	trax_region* tregion = NULL;

	int result = trax_server_wait(handle, &timage, &tregion, properties.properties);

	if (tregion)
		region.wrap(tregion);
	if (timage)
		image.wrap(timage);

	return result;

}

int Server::reply(const Region& region, const Properties& properties) {

	return trax_server_reply(handle, region.region, properties.properties);

}

const Configuration Server::configuration() {

	return handle->config;

}

Image::Image() {
	image = NULL;
}

Image Image::create_path(const std::string& path) {
	Image image;
	image.wrap(trax_image_create_path(path.c_str()));
	return image;
}

Image Image::create_url(const std::string& url) {
	Image image;
	image.wrap(trax_image_create_url(url.c_str()));
	return image;
}

Image Image::create_memory(int width, int height, int format) {
	Image image;
	image.wrap(trax_image_create_memory(width, height, format));
	return image;
}

Image Image::create_buffer(int length, const char* data) {
	Image image;
	image.wrap(trax_image_create_buffer(length, data));
	return image;
}

Image::~Image() {
	release();	
}

int Image::type() const {
	return trax_image_get_type(image);
}

const std::string Image::get_path() const {
	return std::string(trax_image_get_path(image));
}

const std::string Image::get_url() const {
	return std::string(trax_image_get_url(image));
}

void Image::get_memory_header(int* width, int* height, int* format) const {
	trax_image_get_memory_header(image, width, height, format);
}

char* Image::write_memory_row(int row) {
	return trax_image_write_memory_row(image, row);
}

const char* Image::get_memory_row(int row) const {
	return trax_image_get_memory_row(image, row);
}

const char* Image::get_buffer(int* length, int* format) const {
	return trax_image_get_buffer(image, length, format);
}

void Image::cleanup() {
	trax_image_release(&image);
}

void Image::wrap(trax_image* obj) {
	release();
	image = obj;
	acquire();
}

Image& Image::operator=(Image lhs) throw() {
	std::swap(image, lhs.image);
	swap(lhs);
	return *this;
}

Region::Region() {
	region = NULL;
}

Region Region::create_static(int code) {

	Region region;
	region.wrap(trax_region_create_special(code));
	return region;

}

Region Region::create_rectangle(float x, float y, float width, float height) {

	Region region;
	region.wrap(trax_region_create_rectangle(x, y, width, height));
	return region;

}

Region Region::create_polygon(int count) {

	Region region;
	region.wrap(trax_region_create_polygon(count));
	return region;

}

Region::~Region() {
	release();	
}

int Region::type() const  {
	return trax_region_get_type(region);
}

void Region::set(int code) {
	if (type() != TRAX_REGION_SPECIAL || claims() > 1) 
		wrap(trax_region_create_special(code));
	else 
		trax_region_set_special(region, code);
}

int Region::get() const {
	return trax_region_get_special(region);
}

void  Region::set(float x, float y, float width, float height)  {
	if (type() != TRAX_REGION_RECTANGLE || claims() > 1) 
		wrap(trax_region_create_rectangle(x, y, width, height));
	else 
		trax_region_set_rectangle(region, x, y, width, height);
}

void  Region::get(float* x, float* y, float* width, float* height) const {
	trax_region_get_rectangle(region, x, y, width, height);
}

void  Region::set_polygon_point(int index, float x, float y) {
	if (claims() > 1)
		wrap(trax_region_create_polygon(get_polygon_count()));

	trax_region_set_polygon_point(region, index, x, y);
}

void  Region::get_polygon_point(int index, float* x, float* y) const {
	trax_region_get_polygon_point(region, index, x, y);
}

int  Region::get_polygon_count() const {
	return trax_region_get_polygon_count(region);
}

Region Region::bounds() const {
	Region b;
	b.wrap(trax_region_get_bounds(region));
	return b;
}

void Region::cleanup() {
	trax_region_release(&region);
}

void Region::wrap(trax_region* obj) {
	release();
	region = obj;
	acquire();
}

Region& Region::operator=(Region lhs) throw() {
	std::swap(region, lhs.region);
	swap(lhs);
	return *this;
}

Properties::Properties() {
	
}

Properties::~Properties() {
	release();
}

void Properties::clear()  {
	if (!properties) return;
	if (claims() > 1) 
		release();
	else
		trax_properties_clear(properties);
}

void Properties::set(const std::string key, const std::string value)  {
	ensure_unique();
	trax_properties_set(properties, key.c_str(), value.c_str());
}

void Properties::set(const std::string key, int value)  {
	ensure_unique();
	trax_properties_set_int(properties, key.c_str(), value);
}

void Properties::set(const std::string key, float value)  {
	ensure_unique();
	trax_properties_set_float(properties, key.c_str(), value);
}

std::string Properties::get(const std::string key)  {
	if (!properties) return std::string();
	return std::string(trax_properties_get(properties, key.c_str()));
}

int Properties::get(const std::string key, int def)  {
	if (!properties) return def;
	return trax_properties_get_int(properties, key.c_str(), def);
}

float Properties::get(const std::string key, float def)  {
	if (!properties) return def;
	return trax_properties_get_float(properties, key.c_str(), def);
}

void Properties::enumerate(trax_enumerator enumerator, void* object)  {
	if (!properties) return;
	trax_properties_enumerate(properties, enumerator, object);
}

void Properties::cleanup() {
	if (properties)
		trax_properties_release(&properties);
}

void Properties::wrap(trax_properties* obj) {
	release();
	properties = obj;
	acquire();
}

Properties& Properties::operator=(Properties lhs) throw() {
	std::swap(properties, lhs.properties);
	swap(lhs);
	return *this;
}

void copy_enumerator(const char *key, const char *value, const void *obj) {
	trax_properties_set((trax_properties*) obj, key, value);
}

void Properties::ensure_unique() {

	if (!properties) {
		wrap(trax_properties_create());
	} else if (claims() > 1) {
		trax_properties* original = properties;
		wrap(trax_properties_create());
		trax_properties_enumerate(original, copy_enumerator, properties);
	}

}

}