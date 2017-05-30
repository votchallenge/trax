
#include "trax.h"

namespace trax {

Logging::Logging(trax_logging logging) {

	this->callback = logging.callback;
	this->data = logging.data;
	this->flags = logging.flags;

}

Logging::Logging(trax_logger callback, void* data, int flags) {

	this->callback = callback;
	this->data = data;
	this->flags = flags;

}

Logging::~Logging() {

}


Bounds::Bounds() {

	this->left = trax_no_bounds.left;
	this->top = trax_no_bounds.top;
	this->right = trax_no_bounds.right;
	this->bottom = trax_no_bounds.bottom;

}

Bounds::Bounds(trax_bounds bounds) {

	this->left = bounds.left;
	this->top = bounds.top;
	this->right = bounds.right;
	this->bottom = bounds.bottom;

}

Bounds::Bounds(float left, float top, float right, float bottom) {

	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;

}

Bounds::~Bounds() {

}

Wrapper::Wrapper() : pn(NULL) {

}

Wrapper::Wrapper(const Wrapper& count) : pn(count.pn) {

}

Wrapper::~Wrapper() {

}

void Wrapper::swap(Wrapper& lhs) {
	std::swap(pn, lhs.pn);
}

long Wrapper::claims() const {
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

void Wrapper::release() {
	if (NULL != pn) {
		--(*pn);
		if (0 == *pn) {
			cleanup();
			delete pn;
		}
		pn = NULL;
	}
}

Metadata::Metadata() {

	wrap(trax_metadata_create(0, 0, NULL, NULL, NULL));
}

Metadata::Metadata(const Metadata& original) : Wrapper(original) {
	if (original.metadata) acquire();
	metadata = original.metadata;
}

Metadata::Metadata(int region_formats, int image_formats, std::string tracker_name,
	std::string tracker_description, std::string tracker_family) {

	wrap(trax_metadata_create(region_formats, image_formats, 
		(tracker_name.empty()) ? NULL : tracker_name.c_str(),
		(tracker_description.empty()) ? NULL : tracker_description.c_str(),
		(tracker_family.empty()) ? NULL : tracker_family.c_str()
		));

}

Metadata::Metadata(trax_metadata* metadata) {

	wrap(trax_metadata_create(metadata->format_region, metadata->format_image, 
		metadata->tracker_name, metadata->tracker_description, metadata->tracker_family));

}

Metadata::~Metadata() {
	release();
}


int Metadata::image_formats() const {

	return metadata->format_image;

}

int Metadata::region_formats() const {

	return metadata->format_region;

}

std::string Metadata::tracker_name() const {

	return (metadata->tracker_name) ? std::string(metadata->tracker_name) : std::string();

}

std::string Metadata::tracker_description() const {

	return (metadata->tracker_description) ? std::string(metadata->tracker_description) : std::string();
}

std::string Metadata::tracker_family() const {

	return (metadata->tracker_family) ? std::string(metadata->tracker_family) : std::string();
}

void Metadata::cleanup() {

	trax_metadata_release(&metadata);

}

void Metadata::wrap(trax_metadata* obj) {
	if (!obj) return;
	release();
	if (obj) acquire();
	metadata = obj;
}

Metadata& Metadata::operator=(Metadata lhs) throw() {

	std::swap(metadata, lhs.metadata);
	swap(lhs);
	return *this;

}

Handle::Handle() {
	handle = NULL;
}

Handle::Handle(const Handle& original) : Wrapper(original) {
	if (original.handle) acquire();
	handle = original.handle;
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
	if (!obj) return;
	release();
	if (obj) acquire();
	handle = obj;
}

const Metadata Handle::metadata() {

	if (!claims()) return Metadata();

	return Metadata(handle->metadata);

}

bool Handle::terminate() {
	if (!claims()) return -1;

	return trax_terminate(handle) == TRAX_OK;
}

Client::Client(int input, int output, Logging log) {
	wrap(trax_client_setup_file(input, output, log));
}

Client::Client(int server, Logging log, int timeout) {
	wrap(trax_client_setup_socket(server, timeout, log));
}

Client::~Client() {
	// Cleanup done in Handle
}

int Client::wait(Region& region, Properties& properties) {

	if (!claims()) return -1;

	trax_region* tregion = NULL;

	properties.ensure_unique();

	int result = trax_client_wait(handle, &tregion, properties.properties);

	if (tregion)
		region.wrap(tregion);

	return result;

}

int Client::initialize(const Image& image, const Region& region, const Properties& properties) {
	if (!claims()) return -1;

	return trax_client_initialize(handle, image.image, region.region, properties.properties);

}

int Client::frame(const Image& image, const Properties& properties) {
	if (!claims()) return -1;

	return trax_client_frame(handle, image.image, properties.properties);

}

Server::Server(Metadata metadata, Logging log) {

	wrap(trax_server_setup(metadata.metadata, log));

}

Server::~Server() {
	// Cleanup done in Handle
}

int Server::wait(Image& image, Region& region, Properties& properties) {

	if (!claims()) return -1;

	trax_image* timage = NULL;
	trax_region* tregion = NULL;

	properties.ensure_unique();

	int result = trax_server_wait(handle, &timage, &tregion, properties.properties);

	if (tregion)
		region.wrap(tregion);
	if (timage)
		image.wrap(timage);

	return result;

}

int Server::reply(const Region& region, const Properties& properties) {

	if (!claims()) return -1;

	return trax_server_reply(handle, region.region, properties.properties);

}

Image::Image() {
	image = NULL;
}

Image::Image(const Image& original) : Wrapper(original) {
	if (original.image) acquire();
	image = original.image;
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

bool Image::empty() const  {
	return type() == TRAX_IMAGE_EMPTY;
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
	if (!obj) return;
	release();
	image = obj;
	if (image) acquire();
}

Image& Image::operator=(Image lhs) throw() {
	std::swap(image, lhs.image);
	swap(lhs);
	return *this;
}

Region::Region() {
	region = NULL;
}

Region::Region(const Region& original) : Wrapper(original) {
	if (original.region) acquire();
	region = original.region;
}

Region Region::create_special(int code) {

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

bool Region::empty() const  {
	return type() == TRAX_REGION_EMPTY;
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
	if (empty()) return;

	trax_region_get_rectangle(region, x, y, width, height);
}

void  Region::set_polygon_point(int index, float x, float y) {
	if (claims() > 1) {
		wrap(trax_region_convert(region, TRAX_REGION_POLYGON));
	}

	trax_region_set_polygon_point(region, index, x, y);
}

void  Region::get_polygon_point(int index, float* x, float* y) const {
	trax_region_get_polygon_point(region, index, x, y);
}

int  Region::get_polygon_count() const {
	return trax_region_get_polygon_count(region);
}

Region Region::convert(int format) const {
	if (empty()) return Region();

	Region temp;
	temp.wrap(trax_region_convert(region, format));

	return temp;
}

Bounds Region::bounds() const {
	if (empty()) return Bounds();

	return Bounds(trax_region_bounds(region));
}

bool Region::contains(float x, float y) const {
	if (empty()) return false;

	return trax_region_contains(region, x, y) != 0;
}

void Region::cleanup() {
	trax_region_release(&region);
}

float Region::overlap(const Region& region, const Bounds& bounds) const {

	if (empty() || region.empty()) return 0;

	return trax_region_overlap(this->region, region.region, bounds);
}

void Region::wrap(trax_region* obj) {
	if (!obj) return;
	release();
	if (obj) acquire();
	region = obj;
}

Region& Region::operator=(Region lhs) throw() {
	std::swap(region, lhs.region);
	swap(lhs);
	return *this;
}

Region::operator std::string () const {

	std::string result;

	if (!empty()) {
		char* str = trax_region_encode(region);

		if (str) {
			result = std::string(str);
			free(str);
		}

	}
	return result;

}

std::ostream& operator<< (std::ostream& output, const Region& region) {

	if (!region.empty()) {

		char* str = trax_region_encode(region.region);

		if (str) {
			output << str;
			free(str);
		}
	}

	output << std::endl;
	return output;

}

std::istream& operator>> (std::istream& input, Region &region) {

	std::string str;

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(input, true);
	std::streambuf* sb = input.rdbuf();
	bool read = true;

	while (read) {
		int c = sb->sbumpc();
		switch (c) {
		case '\n': {
			read = false;
			break;
		}
		case '\r': {
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			read = false;
			break;
		}
		case EOF: {
			// Also handle the case when the last line has no line ending
			if (str.empty())
				input.setstate(std::ios::eofbit);
			read = false;
			break;
		}
		default:
			str += (char)c;
			break;
		}
	}

	region.wrap(trax_region_decode(str.c_str()));

	return input;
}


Properties::Properties() {
	properties = NULL;
}

Properties::Properties(const Properties& original) : Wrapper(original) {
	if (original.properties) acquire();
	properties = original.properties;
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

std::string Properties::get(const std::string key, const char* def)  {
	return get(key, (def ? std::string(def) : std::string("")));
}

std::string Properties::get(const std::string key, const std::string& def)  {
	if (!properties) return def;
	char* str = trax_properties_get(properties, key.c_str());
	if (str) {
		std::string result(str);
		free(str);
		return result;
	}
	else
		return def;
}

int Properties::get(const std::string key, int def)  {
	if (!properties) return def;
	return trax_properties_get_int(properties, key.c_str(), def);
}

float Properties::get(const std::string key, float def)  {
	if (!properties) return def;
	return trax_properties_get_float(properties, key.c_str(), def);
}

double Properties::get(const std::string key, double def)  {
	return (double) get(key, (float) def);
}

bool Properties::get(const std::string key, bool def)  {
	if (!properties) return def;
	return trax_properties_get_int(properties, key.c_str(), def) != 0;
}

void Properties::enumerate(Enumerator enumerator, void* object)  {
	if (!properties) return;
	trax_properties_enumerate(properties, enumerator, object);
}

void Properties::cleanup() {
	if (properties)
		trax_properties_release(&properties);
}

void Properties::wrap(trax_properties* obj) {
	if (!obj) return;
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

void print_enumerator(const char *key, const char *value, const void *obj) {

	*((std::ostream *) obj) << key << "=" << value << std::endl;

}

void map_enumerator(const char *key, const char *value, const void *obj) {

	(*((std::map<std::string, std::string>*) obj))[std::string(key)] = std::string(value);

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

std::ostream& operator<< (std::ostream& output, const Properties& properties) {

	if (!properties.properties) return output;
	trax_properties_enumerate(properties.properties, print_enumerator, &output);
	return output;
}

void Properties::from_map(const std::map<std::string, std::string>& m) {
	typedef std::map<std::string, std::string>::const_iterator it_type;
	for(it_type iterator = m.begin(); iterator != m.end(); iterator++) {
		set(iterator->first, iterator->second);
	}
}

void Properties::to_map(std::map<std::string, std::string>& m) const {
	
	if (!properties) return;

	trax_properties_enumerate(properties, map_enumerator, &m);

}

}