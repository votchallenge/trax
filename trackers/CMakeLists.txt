CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_EXECUTABLE(native_static native_static.c)
TARGET_LINK_LIBRARIES(native_static traxstatic)

IF (BUILD_OPENCV)
	ADD_EXECUTABLE(camshift camshift.cpp)
	ADD_EXECUTABLE(interactive interactive.cpp)
	TARGET_LINK_LIBRARIES(camshift trax trax_opencv ${OpenCV_LIBS})
	TARGET_LINK_LIBRARIES(interactive trax trax_opencv ${OpenCV_LIBS})
ENDIF (BUILD_OPENCV)