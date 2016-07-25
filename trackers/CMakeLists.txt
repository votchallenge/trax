CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_EXECUTABLE(native_static native_static.c)
TARGET_LINK_LIBRARIES(native_static traxstatic)
