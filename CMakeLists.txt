PROJECT (trax)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
GET_FILENAME_COMPONENT(PROJECT_ROOT . ABSOLUTE)
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
INCLUDE(GNUInstallDirs)
INCLUDE(CMakePackageConfigHelpers)

SET(CPACK_GENERATOR "ZIP")
SET(CPACK_PACKAGE_NAME trax)
SET(CPACK_PACKAGE_VENDOR "Luka Cehovin Zajc and VOT Innitiative")
SET(CPACK_PACKAGE_CONTACT "Luka Cehovin Zajc <luka.cehovin@fri.uni-lj.si>")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luka Cehovin Zajc <luka.cehovin@fri.uni-lj.si>")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
SET(CPACK_INSTALL_PREFIX "")

SET (BUILD_DEBUG FALSE CACHE BOOL "Build in debug mode")

IF (NOT BUILD_DATE)
	STRING(TIMESTAMP BUILD_DATE "%Y-%m-%d")
ENDIF()

IF (NOT BUILD_VERSION)
    FILE(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" BUILD_VERSION)
	STRING(REPLACE "\n" "" BUILD_VERSION ${BUILD_VERSION})
ENDIF()

IF (WIN32 OR WIN64)
    SET(LIBRARIES ws2_32)
	SET(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}")
	SET(CMAKE_DEBUG_POSTFIX "d")
ELSE ()
    SET(LIBRARIES m)
	SET(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/trax")
	SET(CPACK_SET_DESTDIR 1)
ENDIF ()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

LINK_LIBRARIES(${LIBRARIES})
ADD_DEFINITIONS(-DTRAX_BUILD_VERSION="${BUILD_VERSION}" -DTRAX_BUILD_DATE="${BUILD_DATE}")
SET(TRAX_SOURCE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/trax.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/region.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/strmap.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/message.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/base64.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/traxpp.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/debug.c)

SET(TRAX_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/trax.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/region.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/strmap.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/message.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/base64.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h)

IF (BUILD_DEBUG)
ADD_DEFINITIONS(-DTRAX_DEBUG=1)
ENDIF ()

ADD_LIBRARY(trax SHARED ${TRAX_SOURCE})
ADD_LIBRARY(traxstatic STATIC ${TRAX_SOURCE})

if(MSVC)
    ADD_DEFINITIONS(/MP) # Build with multiple processes
    ADD_DEFINITIONS(/wd4244 /wd4267 /wd4018 /wd4355 /wd4800 /wd4251 /wd4996 /wd4146 /wd4305) # Ignore some of the warnings
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
    IF (BUILD_DEBUG)
        SET_TARGET_PROPERTIES(trax PROPERTIES COMPILE_FLAGS "-finstrument-functions")
        SET_TARGET_PROPERTIES(traxstatic PROPERTIES COMPILE_FLAGS "-finstrument-functions")
        ADD_SUBDIRECTORY(src/tests)
    ENDIF ()
endif()

TARGET_COMPILE_OPTIONS(traxstatic PUBLIC -DTRAX_STATIC_DEFINE)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
SET_TARGET_PROPERTIES(traxstatic PROPERTIES POSITION_INDEPENDENT_CODE 1)

SET (BUILD_TESTS FALSE CACHE BOOL "Build with support for tests")
SET (BUILD_OPENCV FALSE CACHE BOOL "Build with OpenCV support")
SET (BUILD_CLIENT FALSE CACHE BOOL "Build with client support library and executable")
SET (BUILD_MATLAB FALSE CACHE BOOL "Build Matlab/Octave bindings")
SET (BUILD_PYTHON FALSE CACHE BOOL "Build Python wrapper")

IF (BUILD_OPENCV)
	FIND_PACKAGE(OpenCV REQUIRED)
	ADD_SUBDIRECTORY(support/opencv)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/support/opencv/include)
ENDIF (BUILD_OPENCV)

IF (BUILD_CLIENT)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    ENDIF ()
	ADD_SUBDIRECTORY(support/client)
ENDIF (BUILD_CLIENT)

IF (BUILD_MATLAB)
	ADD_SUBDIRECTORY(support/matlab)
ENDIF (BUILD_MATLAB)

IF (WIN32 OR WIN64)
	INSTALL(TARGETS trax traxstatic EXPORT core RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
ELSE ()
	INSTALL(TARGETS trax EXPORT core LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDIF()

INSTALL(FILES include/trax.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
INSTALL(EXPORT core DESTINATION ${CONFIG_INSTALL_DIR} FILE trax-component-core.cmake)

IF (BUILD_PYTHON)
    ADD_SUBDIRECTORY(support/python)	
ENDIF (BUILD_PYTHON)

IF (BUILD_TESTS)
    INCLUDE( CTest )
	ADD_SUBDIRECTORY(support/tests)
ENDIF (BUILD_TESTS)

# CMake config file
SET(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}")

CONFIGURE_PACKAGE_CONFIG_FILE(trax-config.cmake.in
    ${PROJECT_BINARY_DIR}/trax-config.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
    PATH_VARS LIB_INSTALL_DIR INCLUDE_INSTALL_DIR)

WRITE_BASIC_PACKAGE_VERSION_FILE(
    ${PROJECT_BINARY_DIR}/trax-config-version.cmake
    VERSION ${BUILD_VERSION}
    COMPATIBILITY SameMajorVersion)

INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/trax-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/trax-config-version.cmake
    DESTINATION ${CONFIG_INSTALL_DIR})

STRING(REPLACE "." ";" VERSION_LIST ${BUILD_VERSION})
LIST(GET VERSION_LIST 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_LIST 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_LIST 2 CPACK_PACKAGE_VERSION_PATCH)
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
IF (BUILD_PLATFORM)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${BUILD_PLATFORM}")
ELSE()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
ENDIF()
INCLUDE(CPack)

SET_TARGET_PROPERTIES(trax PROPERTIES VERSION ${BUILD_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR})