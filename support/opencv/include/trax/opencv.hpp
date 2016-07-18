
#ifndef __TRAX_OPENCV
#define __TRAX_OPENCV

#include <opencv2/core/core.hpp>
#include <trax.h>

namespace trax {
	
cv::Mat image_to_mat(const trax_image* image);

cv::Rect region_to_rect(const trax_region* region);

trax_image* mat_to_image(const cv::Mat& mat);

trax_region* rect_to_region(const cv::Rect rect);

}

#endif
