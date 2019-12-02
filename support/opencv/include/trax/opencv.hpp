
#ifndef __TRAX_OPENCV
#define __TRAX_OPENCV

#include <opencv2/core/core.hpp>
#include <trax.h>

#ifdef TRAX_OPENCV_STATIC_DEFINE
#  define __TRAX_OPENCV_EXPORT
#else
#  ifndef __TRAX_OPENCV_EXPORT
#    if defined(_MSC_VER)
#      ifdef trax_opencv_EXPORTS 
         /* We are building this library */
#        define __TRAX_OPENCV_EXPORT __declspec(dllexport)
#      else
         /* We are using this library */
#        define __TRAX_OPENCV_EXPORT __declspec(dllimport)
#      endif
#    elif defined(__GNUC__)
#      ifdef trax_opencv_EXPORTS
         /* We are building this library */
#        define __TRAX_OPENCV_EXPORT __attribute__((visibility("default")))
#      else
         /* We are using this library */
#        define __TRAX_OPENCV_EXPORT __attribute__((visibility("default")))
#      endif
#    endif
#  endif
#endif

namespace trax {
	
__TRAX_OPENCV_EXPORT cv::Mat image_to_mat(const Image& image);

__TRAX_OPENCV_EXPORT cv::Rect region_to_rect(const Region& region);

__TRAX_OPENCV_EXPORT std::vector<cv::Point2f> region_to_points(const Region& region);

__TRAX_OPENCV_EXPORT cv::Mat region_to_mat(const Region& region);

__TRAX_OPENCV_EXPORT Image mat_to_image(const cv::Mat& mat);

__TRAX_OPENCV_EXPORT Region rect_to_region(const cv::Rect rect);

__TRAX_OPENCV_EXPORT Region points_to_region(const std::vector<cv::Point2f> points);

__TRAX_OPENCV_EXPORT Region mat_to_region(const cv::Mat& mat);

__TRAX_OPENCV_EXPORT void draw_region(cv::Mat& canvas, const Region& region, cv::Scalar color, int width = 1);

}

#endif
