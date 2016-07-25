
#include <trax/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>

namespace trax {

cv::Mat image_to_mat(const Image& image) {
	
    switch (image.type()) {
    case TRAX_IMAGE_PATH:
        return cv::imread(image.get_path(), CV_LOAD_IMAGE_COLOR);
    case TRAX_IMAGE_MEMORY: {
        int width, height, format;
        image.get_memory_header(&width, &height, &format);
        const char* data = image.get_memory_row(0);

        cv::Mat tmp(height, width, format == TRAX_IMAGE_MEMORY_RGB ? CV_8UC3 : 
            (format == TRAX_IMAGE_MEMORY_GRAY8 ? CV_8UC1 : CV_16U), const_cast<char *>(data));

        cv::Mat result;

        cv::cvtColor(tmp, result, CV_BGR2RGB);
        
        return result;
    }
    case TRAX_IMAGE_BUFFER: {
        int length, format;
        const char* buffer = image.get_buffer(&length, &format);

        // This is problematic but we will just read from the buffer anyway ...
        const cv::Mat mem(1, length, CV_8UC1, const_cast<char *>(buffer)); 

        return cv::imdecode(mem, CV_LOAD_IMAGE_COLOR);

    }
    }
    return cv::Mat();
}

cv::Rect region_to_rect(const Region& region) {
	
	float x, y, width, height;
	cv::Rect rect;

	Region trect = region.convert(TRAX_REGION_RECTANGLE);

	trect.get(&(x), &(y), &(width), &(height));

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height; 

	return rect;

}

Image mat_to_image(const cv::Mat& mat) {
	
    int format = 0;
    switch(mat.type()) {
    case CV_8UC1:
        format = TRAX_IMAGE_MEMORY_GRAY8;
        break;
    case CV_16UC1:
        format = TRAX_IMAGE_MEMORY_GRAY16;
        break;
    case CV_8UC3:
        format = TRAX_IMAGE_MEMORY_RGB;
        break;
    default:
        throw std::runtime_error("Unsupported image depth");
    }
    Image image = Image::create_memory(mat.cols, mat.rows, format);
    char* dst = image.write_memory_row( 0);
    cv::Mat tmp(mat.size(), mat.type(), dst);
    cv::cvtColor(mat, tmp, CV_BGR2RGB);

    return image;
}

Region rect_to_region(const cv::Rect rect) {
	
	return Region::create_rectangle(rect.x, rect.y, rect.width, rect.height);

}

}