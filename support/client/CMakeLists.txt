CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_LIBRARY(trax_client SHARED client.cpp process.cpp threads.c)
TARGET_LINK_LIBRARIES(trax_client trax ${OpenCV_LIBS})
INSTALL(FILES "include/trax/client.hpp" DESTINATION include/trax)

ADD_EXECUTABLE(traxclient getopt.c cli.cpp)
TARGET_LINK_LIBRARIES(traxclient traxstatic trax_client)
IF (BUILD_OPENCV)
  TARGET_LINK_LIBRARIES(traxclient trax_opencv ${OpenCV_LIBS})
ENDIF (BUILD_OPENCV)

IF (WIN32 OR WIN64)
	INSTALL(TARGETS trax_client traxclient RUNTIME DESTINATION bin)
ELSE ()
	INSTALL(TARGETS traxclient RUNTIME DESTINATION bin)
	INSTALL(TARGETS trax_client LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ENDIF()