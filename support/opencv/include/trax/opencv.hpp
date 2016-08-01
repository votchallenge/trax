
#ifndef __TRAX_OPENCV
#define __TRAX_OPENCV

#include <opencv2/core/core.hpp>
#include <trax.h>

namespace trax {
	
cv::Mat image_to_mat(const Image& image);

cv::Rect region_to_rect(const Region& region);

Image mat_to_image(const cv::Mat& mat);

Region rect_to_region(const cv::Rect rect);

void draw_region(cv::Mat& canvas, const Region& region, cv::Scalar color, int width = 1);

}

#endif
