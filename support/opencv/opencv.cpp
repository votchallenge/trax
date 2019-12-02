
#include <trax/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <vector>
#include <stdexcept>

#if defined (_MSC_VER) && _MSC_VER < 1800
#define round(fp) (int)((fp) >= 0 ? (fp) + 0.5 : (fp) - 0.5)
#endif

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

	rect.x = round(x);
	rect.y = round(y);
	rect.width = round(width);
	rect.height = round(height); 

	return rect;

}

std::vector<cv::Point2f> region_to_points(const Region& region) {

    std::vector<cv::Point2f> points;

    Region polygon = region.convert(TRAX_REGION_POLYGON);

    if (polygon.type() != TRAX_REGION_POLYGON) return points;

    for (int i = 0; i < polygon.get_polygon_count(); i++) {
        cv::Point2f p;
        polygon.get_polygon_point(i, &(p.x), &(p.y));
        points.push_back(p);
    }

    return points;

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
    char* dst = image.write_memory_row(0);
    cv::Mat tmp(mat.size(), mat.type(), dst);
    cv::cvtColor(mat, tmp, CV_BGR2RGB);

    return image;
}

Region rect_to_region(const cv::Rect rect) {
	
	return Region::create_rectangle(rect.x, rect.y, rect.width, rect.height);

}

Region points_to_region(const std::vector<cv::Point2f> points) {
	size_t i;
    Region polygon = Region::create_polygon(points.size());

    for (i = 0; i < points.size(); i++) {
        polygon.set_polygon_point(i, points[i].x, points[i].y);
    }

    return polygon;

}

cv::Mat region_to_mat(const Region& region) {

    int height, width, x, y;
    region.get_mask_header(&x, &y, &width, &height);

    cv::Mat result(height + y, width + x, CV_8UC1);

    for (int i = 0; i < height; i++) {
            const char* data = region.get_mask_row(i);
            memcpy(&(result.data[(i + y) * width + x]), data, width);

    }
    return result;

}

Region mat_to_region(const cv::Mat& mat) {

    CV_Assert(mat.type() == CV_8UC1);

    Region r = Region::create_mask(0, 0, mat.cols, mat.rows);
    memcpy(r.write_mask_row(0), mat.data, mat.cols * mat.rows);

    return r;

}

void draw_region(cv::Mat& canvas, const Region& region, cv::Scalar color, int width) {

    if (region.empty()) return;

    switch (region.type()) {
        case TRAX_REGION_RECTANGLE: {
            cv::Rect rectangle = region_to_rect(region);
            cv::rectangle(canvas, rectangle.tl(), rectangle.br(), color, width);
            break;
        }
        case TRAX_REGION_POLYGON: {
            std::vector<cv::Point> points(region.get_polygon_count());

            for (int i = 0; i < region.get_polygon_count(); i++) {
                float x, y;
                region.get_polygon_point(i, &x, &y);
                points[i] = cv::Point(x, y);
            }

            const cv::Point* ppoints = &points[0];
            int npoints = (int) points.size();

            if (width < 1) 
                cv::fillPoly(canvas, &ppoints, &npoints, 1, color);
            else
                cv::polylines(canvas, &ppoints, &npoints, 1, true, color, width);

            break;
        }
        case TRAX_REGION_MASK: {

            cv::Mat mask = region_to_mat(region);

            int width = MIN(canvas.cols, mask.cols);
            int height = MIN(canvas.rows, mask.rows);

            cv::Mat stencil = cv::Mat::zeros(cv::Size(height, width), CV_8UC3);

            stencil(cv::Range(0, height), cv::Range(0, width)).setTo(color, mask);

            canvas = canvas * (1 - stencil) + stencil;

            break;
        }
    }
}

}