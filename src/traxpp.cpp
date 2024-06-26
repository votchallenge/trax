
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
	acquire();
}


Wrapper::Wrapper(Wrapper&& count) : pn(count.pn) {
	count.pn = NULL;
}

Wrapper::~Wrapper() {

}

void Wrapper::acquire(const Wrapper& lhs) {
	release();
	pn = lhs.pn;
	acquire();
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
    wrap(trax_metadata_create(0, 0, TRAX_CHANNEL_COLOR, NULL, NULL, NULL, 0));
}

Metadata::Metadata(Metadata&& original) : Wrapper(std::move(original)) {
	metadata = original.metadata;
}

Metadata::Metadata(const Metadata& original) : Wrapper(original) {
	metadata = original.metadata;
}

Metadata::Metadata(int region_formats, int image_formats, int channels,
                   std::string tracker_name, std::string tracker_description,
                   std::string tracker_family, int flags) {

    wrap(trax_metadata_create(region_formats, image_formats, channels,
		(tracker_name.empty()) ? NULL : tracker_name.c_str(),
		(tracker_description.empty()) ? NULL : tracker_description.c_str(),
		(tracker_family.empty()) ? NULL : tracker_family.c_str(), flags
		));

}

Metadata::Metadata(trax_metadata* metadata) {

    wrap(trax_metadata_create(metadata->format_region, metadata->format_image, metadata->channels,
		metadata->tracker_name, metadata->tracker_description, metadata->tracker_family, metadata->flags));

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

int Metadata::channels() const {

	return metadata->channels;

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

Metadata& Metadata::operator=(const Metadata& lhs) throw() {
	acquire(lhs);
	metadata = lhs.metadata;
	return *this;
}

Metadata& Metadata::operator=(Metadata&& lhs) throw() {
	swap(lhs);
	std::swap(metadata, lhs.metadata);
	return *this;
}

std::string Metadata::get_custom(const std::string key) const {

	char* cval = trax_properties_get(metadata->custom, key.c_str());

	if (!cval) return std::string();

	std::string val(cval);
	free(cval);
	return val;

}

void Metadata::set_custom(const std::string key, const std::string value) {

	trax_properties_set(metadata->custom, key.c_str(), value.c_str());

}

Handle::Handle() {
	handle = NULL;
}

Handle::Handle(const Handle& original) : Wrapper(original) {
	handle = original.handle;
}

Handle::Handle(Handle&& original) : Wrapper(original) {
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

bool Handle::terminate(const std::string reason) {
	if (!claims()) return -1;
	return trax_terminate(handle, reason.c_str()) == TRAX_OK;
}

std::string Handle::get_error() {
	if (!claims()) return std::string();

	const char* error = trax_get_error(handle);

	if (!error) return std::string();

	return std::string(error);
}

bool Handle::is_alive() {
	if (!claims()) return false;

	return trax_is_alive(handle) != 0;
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

	trax_object_list* tobjects = NULL;
	trax_region* tregion = NULL;

	properties.ensure_unique();

	int result = trax_client_wait(handle, &tobjects, properties.properties);

	if (tobjects) {
		if (trax_object_list_count(tobjects) > 1) {
			trax_object_list_release(&tobjects);
			return TRAX_ERROR;
		}

		if (trax_object_list_count(tobjects) == 1) {
			tregion = trax_region_clone(trax_object_list_get(tobjects, 0));
			region.wrap(tregion);
			trax_properties_append(trax_object_list_properties(tobjects, 0), properties.properties, 0);
		}
		trax_object_list_release(&tobjects);
	}
	
	return result;

}

int Client::wait(ObjectList& objects, Properties& properties) {

	if (!claims()) return -1;

	trax_object_list* tobjects = NULL;

	properties.ensure_unique();

	int result = trax_client_wait(handle, &tobjects, properties.properties);

	if (tobjects) {
		objects.wrap(tobjects);
	}

	return result;

}

int Client::initialize(const ImageList& image, const Region& region, const Properties& properties) {
	if (!claims()) return -1;

	trax_object_list* objects = trax_object_list_create(1);
	trax_object_list_set(objects, 0, region.region);

	int result = trax_client_initialize(handle, image.list, objects, properties.properties);

	trax_object_list_release(&objects);

	return result;

}

int Client::initialize(const ImageList& image, const ObjectList& objects, const Properties& properties) {
	if (!claims()) return -1;

	return trax_client_initialize(handle, image.list, objects.list, properties.properties);

}

int Client::frame(const ImageList& image, const Properties& properties) {
	if (!claims()) return -1;

	return trax_client_frame(handle, image.list, NULL, properties.properties);

}

int Client::frame(const ImageList& image, const ObjectList& objects, const Properties& properties) {

	if (!claims()) return -1;

	return trax_client_frame(handle, image.list, objects.list, properties.properties);

}

ServerSOT::ServerSOT(Metadata metadata, Logging log) {

	wrap(trax_server_setup_v(metadata.metadata, log, 3));

}

ServerSOT::~ServerSOT() {
	// Cleanup done in Handle
}

int ServerSOT::wait(ImageList& image, Region& region, Properties& properties) {

	if (!claims()) return -1;

	trax_region* tregion = NULL;

    trax_image_list* timagelist = NULL;

	properties.ensure_unique();

    int result = trax_server_wait_sot(handle, &timagelist, &tregion, properties.properties);

	if (tregion)
		region.wrap(tregion);

	if (timagelist) {
		image.wrap(timagelist);		
	}

	return result;

}

int ServerSOT::reply(const Region& region, const Properties& properties) {

	if (!claims()) return -1;

	return trax_server_reply_sot(handle, region.region, properties.properties);

}

ServerMOT::ServerMOT(Metadata metadata, Logging log) {

	wrap(trax_server_setup_v(metadata.metadata, log, 0));

}

ServerMOT::~ServerMOT() {
	// Cleanup done in Handle
}

int ServerMOT::wait(ImageList& image, Region& region, Properties& properties) {

	if (!claims()) return -1;

	trax_object_list* tobjects = NULL;
    trax_image_list* timagelist = NULL;
	trax_region* tregion = NULL;

	properties.ensure_unique();

    int result = trax_server_wait_mot(handle, &timagelist, &tobjects, properties.properties);

	if (timagelist) {
		image.wrap(timagelist);		
	}

	if (tobjects) {
		if (trax_object_list_count(tobjects) > 1) {
			trax_object_list_release(&tobjects);
			return TRAX_ERROR;
		}
		if (trax_object_list_count(tobjects) == 1) {
			tregion = trax_region_clone(trax_object_list_get(tobjects, 0));
			region.wrap(tregion);
			trax_properties_append(trax_object_list_properties(tobjects, 0), properties.properties, 0);
		}
	}

	return result;

}

int ServerMOT::reply(const Region& region, const Properties& properties) {

	if (!claims()) return -1;

	trax_object_list* objects = trax_object_list_create(1);
	trax_object_list_set(objects, 0, region.region);
	trax_properties_append(trax_object_list_properties(objects, 0), properties.properties, 0);

	int result = trax_server_reply_mot(handle, objects);

	trax_object_list_release(&objects);

	return result;

}

int ServerMOT::wait(ImageList& image, ObjectList& objects, Properties& properties) {

	trax_object_list* tobjects = NULL;
    trax_image_list* timagelist = NULL;

	properties.ensure_unique();

	int result = trax_server_wait_mot(handle, &timagelist, &tobjects, properties.properties);

	if (timagelist) {
		image.wrap(timagelist);		
	}

	if (tobjects) {
		objects.wrap(tobjects);
	}

	return result;

}


int ServerMOT::reply(const ObjectList& objects) {

	if (!claims()) return -1;

	return trax_server_reply_mot(handle, objects.list);

}

Image::Image() {
	image = NULL;
}

Image::Image(const Image& original) : Wrapper(original) {
	image = original.image;
}

Image::Image(Image&& original) : Wrapper(std::move(original)) {
	image = original.image;
}

Image& Image::operator=(const Image& lhs) throw() {
	acquire(lhs);
	image = lhs.image;
	return *this;
}

Image& Image::operator=(Image&& lhs) throw() {
	swap(lhs);
	std::swap(image, lhs.image);
	return *this;
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
	if (!image) return;
	trax_image_release(&image);
}

void Image::wrap(trax_image* obj) {
	if (!obj) return;
	release();
	image = obj;
	if (image) acquire();
}

ObjectList::ObjectList() {
	list = NULL;
}

ObjectList::ObjectList(int count) {
	list = trax_object_list_create(count);
	_regions.resize(count);
	_properties.resize(count);
}

ObjectList::ObjectList(const ObjectList& original) : Wrapper(original) {
	list = original.list;
	_regions = original._regions;
	_properties = original._properties;
}

ObjectList::ObjectList(ObjectList&& original) : Wrapper(std::move(original)) {
	std::swap(list, original.list);
	std::swap(_regions, original._regions);
	std::swap(_properties, original._properties);
}


ObjectList& ObjectList::operator=(const ObjectList& original) throw() {
	acquire(original);
	list = original.list;
	_regions = original._regions;
	_properties = original._properties;
	return *this;
}

ObjectList& ObjectList::operator=(ObjectList&& original) throw() {
	swap(original);
	std::swap(list, original.list);
	std::swap(_regions, original._regions);
	std::swap(_properties, original._properties);
	return *this;
}

ObjectList::~ObjectList() {
	release();
}

Region ObjectList::get(int index) const {
	return _regions[index];
}

void ObjectList::set(int index, Region region) {
	_regions[index] = region;
	list->regions[index] = region.region;
	// Do not use C function as it will try to free old region
	// trax_object_list_set(list, index, region.region);
}

Properties ObjectList::properties(int index) const {
	return _properties[index];
}

int ObjectList::size() const {
	if (!list) return 0;
	return trax_object_list_count(list);
}


void ObjectList::cleanup() {
	if (!list) return;
	// Only free the structure, regions and properties will be 
	// released by proxies
	free(list->regions);
	free(list->properties);
	free(list);
	_regions.clear();
	_properties.clear();
}

void ObjectList::wrap(trax_object_list* obj) {
	if (!obj) return;
	release();
	list = obj;
	if (list) acquire();

	_regions.resize(trax_object_list_count(obj));
	_properties.resize(trax_object_list_count(obj));

	for (int i = 0; i < trax_object_list_count(obj); i++) {
		_regions[i].wrap(obj->regions[i]);
		_properties[i].wrap(obj->properties[i]);
	}
}


ImageList::ImageList()  {
	list = trax_image_list_create();
	images.resize(TRAX_CHANNELS);
}

ImageList::ImageList(const ImageList& original)  : Wrapper(original) {
	list = original.list;
	images = original.images;
}

ImageList::ImageList(ImageList&& original)  : Wrapper(std::move(original)) {
	std::swap(list, original.list);
	std::swap(images, original.images);
}

ImageList& ImageList::operator=(const ImageList& original) throw() {
	acquire(original);
	list = original.list;
	images = original.images;
	return *this;
}

ImageList& ImageList::operator=(ImageList&& original) throw() {
	swap(original);
	std::swap(list, original.list);
	std::swap(images, original.images);
	return *this;
}

ImageList::~ImageList() {
	release();
}

Image ImageList::get(int channel) const {

	return images[TRAX_CHANNEL_INDEX(channel)];

}

bool ImageList::has(int channel) const {

	return !images[TRAX_CHANNEL_INDEX(channel)].empty();

}

void ImageList::set(Image image, int channel) {

	images[TRAX_CHANNEL_INDEX(channel)] = image;
	list->images[TRAX_CHANNEL_INDEX(channel)] = image.image;

}

int ImageList::size() const {

	int count = 0;

	for (int i = 0; i < TRAX_CHANNELS; i++) {

		if (!images[i].empty())
			count++;

	}

	return count;
}

void ImageList::cleanup() {
	if (!list) return;
	trax_image_list_release(&list);

}

void ImageList::wrap(trax_image_list* obj) {

	if (!obj) return;
	release();
	list = obj;
	if (list) acquire();

	for (int i = 0; i < TRAX_CHANNELS; i++)
		images[i].wrap(obj->images[i]);

}


Region::Region() {
	region = NULL;
}

Region::Region(const Region& original) : Wrapper(original) {
	region = original.region;
}

Region::Region(Region&& original) : Wrapper(std::move(original)) {
	std::swap(region, original.region);
}

Region& Region::operator=(const Region& original) throw() {
	acquire(original);
	region = original.region;
	return *this;
}

Region& Region::operator=(Region&& original) throw() {
	swap(original);
	std::swap(region, original.region);
	return *this;
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

Region Region::create_mask(int x, int y, int width, int height) {

	Region region;
	region.wrap(trax_region_create_mask(x, y, width, height));
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

void Region::get_polygon_point(int index, float* x, float* y) const {
	trax_region_get_polygon_point(region, index, x, y);
}

int Region::get_polygon_count() const {
	return trax_region_get_polygon_count(region);
}

void Region::get_mask_header(int* x, int* y, int* width, int* height) const {

	trax_region_get_mask_header(region, x, y, width, height);

}

char* Region::write_mask_row(int row) {

	return trax_region_write_mask_row(region, row);

}

const char* Region::get_mask_row(int row) const {

	return trax_region_get_mask_row(region, row);

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
	if (!region) return;
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
	properties = original.properties;
}

Properties::Properties(Properties&& original) : Wrapper(std::move(original)) {
	std::swap(properties, original.properties);
}

Properties& Properties::operator=(const Properties& original) throw() {
	acquire(original);
	properties = original.properties;
	return *this;
}

Properties::~Properties() {
	release();
}

int Properties::size() const {
	if (!properties) return 0;
	return trax_properties_count(properties);
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

std::string Properties::get(const std::string key, const char* def) const {
	return get(key, (def ? std::string(def) : std::string("")));
}

std::string Properties::get(const std::string key, const std::string& def) const {
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

int Properties::get(const std::string key, int def) const {
	if (!properties) return def;
	return trax_properties_get_int(properties, key.c_str(), def);
}

float Properties::get(const std::string key, float def) const {
	if (!properties) return def;
	return trax_properties_get_float(properties, key.c_str(), def);
}

double Properties::get(const std::string key, double def) const {
	return (double) get(key, (float) def);
}

bool Properties::get(const std::string key, bool def) const {
	if (!properties) return def;
	return trax_properties_get_int(properties, key.c_str(), def) != 0;
}

void Properties::enumerate(Enumerator enumerator, void* object)  {
	if (!properties) return;
	trax_properties_enumerate(properties, enumerator, object);
}

void Properties::cleanup() {
	if (!properties) return;
	trax_properties_release(&properties);
}

void Properties::wrap(trax_properties* obj) {
	if (!obj) return;
	release();
	properties = obj;
	acquire();
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

void vector_enumerator(const char *key, const char *value, const void *obj) {

	(*((std::vector<std::string>*) obj)).push_back(std::string(key));

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

void Properties::to_vector(std::vector<std::string>& v) const {

	if (!properties) return;

	trax_properties_enumerate(properties, vector_enumerator, &v);

}

}
