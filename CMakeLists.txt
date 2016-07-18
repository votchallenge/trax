PROJECT (trax)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
GET_FILENAME_COMPONENT(PROJECT_ROOT . ABSOLUTE)

IF (NOT BUILD_VERSION)
    FIND_PACKAGE(Git)
    # Try to find the Git commit and use it as version identifier
    IF(GIT_FOUND)
       EXECUTE_PROCESS(
       COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
       WORKING_DIRECTORY ${PROJECT_ROOT}
       OUTPUT_VARIABLE BUILD_VERSION
       OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
        SET(BUILD_VERSION "unknown")
    ENDIF()
ENDIF()

SET (BUILD_OPENCV FALSE CACHE BOOL "Build using OpenCV (example and image loading)")

IF (BUILD_OPENCV)
	FIND_PACKAGE(OpenCV REQUIRED)
	INCLUDE_DIRECTORIES(AFTER ${OpenCV_INCLUDE_DIR})
  ADD_DEFINITIONS(-DTRAX_BUILD_OPENCV)
ENDIF (BUILD_OPENCV)

ADD_DEFINITIONS(-DTRAX_BUILD_VERSION="${BUILD_VERSION}")

IF (WIN32 OR WIN64)
    SET(LIBRARIES ws2_32)
ELSE ()
    SET(LIBRARIES m)
    # Only build test executable on Unix systems / used only for development
    ADD_EXECUTABLE(traxtest lib/test.c)
    TARGET_LINK_LIBRARIES(traxtest trax)
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    ENDIF ()    
ENDIF ()

LINK_LIBRARIES(${LIBRARIES})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

ADD_LIBRARY(trax SHARED lib/trax.c lib/region.c lib/strmap.c lib/message.c lib/base64.c)

ADD_LIBRARY(traxstatic STATIC lib/trax.c lib/region.c lib/strmap.c lib/message.c lib/base64.c)
SET_TARGET_PROPERTIES(traxstatic PROPERTIES POSITION_INDEPENDENT_CODE 1)

ADD_EXECUTABLE(native_static trackers/native_static.c)
TARGET_LINK_LIBRARIES(native_static traxstatic)

IF (BUILD_OPENCV)
	ADD_EXECUTABLE(camshift trackers/camshift.cpp)
	TARGET_LINK_LIBRARIES(camshift trax ${OpenCV_LIBS})
ENDIF (BUILD_OPENCV)

SET (BUILD_CLIENT FALSE CACHE BOOL "Build a client executable")

IF (BUILD_CLIENT)
    IF (WIN32 OR WIN64)
        ADD_EXECUTABLE(traxclient client/client.cpp client/process.cpp client/threads.c client/utilities.cpp client/getopt_win.c)
    ELSE ()
        ADD_EXECUTABLE(traxclient client/client.cpp client/process.cpp client/threads.c client/utilities.cpp)
    ENDIF ()
    TARGET_LINK_LIBRARIES(traxclient traxstatic)
ENDIF (BUILD_CLIENT)

IF (WIN32 OR WIN64)
	INSTALL(TARGETS trax RUNTIME DESTINATION bin)
	INSTALL(TARGETS traxstatic LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ELSE ()
	INSTALL(TARGETS trax traxstatic LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ENDIF()

INSTALL(FILES lib/trax.h DESTINATION include)
IF (BUILD_CLIENT)
	INSTALL(TARGETS traxclient RUNTIME DESTINATION bin)
	IF (BUILD_OPENCV)
		TARGET_LINK_LIBRARIES(traxclient ${OpenCV_LIBS})
	ENDIF (BUILD_OPENCV)
ENDIF (BUILD_CLIENT)

