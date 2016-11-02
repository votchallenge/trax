CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_LIBRARY(trax_client SHARED client.cpp process.cpp threads.c)
TARGET_LINK_LIBRARIES(trax_client trax)
INSTALL(FILES "include/trax/client.hpp" DESTINATION include/trax)

# Temporarily avoiding linking with trax_client lib because of VOT toolkit
# this has to be resolved in some other way someday
ADD_EXECUTABLE(traxclient getopt.c cli.cpp client.cpp process.cpp threads.c timer.cpp)
TARGET_LINK_LIBRARIES(traxclient traxstatic)
IF (BUILD_OPENCV)
	ADD_DEFINITIONS(-DTRAX_BUILD_OPENCV)
  	TARGET_LINK_LIBRARIES(traxclient trax_opencv ${OpenCV_LIBS})
ENDIF (BUILD_OPENCV)

IF (WIN32 OR WIN64)
	INSTALL(TARGETS trax_client traxclient traxplayer RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
ELSE ()
	ADD_EXECUTABLE(traxplayer player.cpp)
	TARGET_LINK_LIBRARIES(traxplayer trax_opencv trax_client ${OpenCV_LIBS})
	INSTALL(TARGETS traxclient traxplayer RUNTIME DESTINATION bin)
	INSTALL(TARGETS trax_client LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ENDIF()
