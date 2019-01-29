

IF (OCTAVE)
ADD_TEST(NAME test_octave_client COMMAND ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "simple_client('./native_static', false)" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(test_octave_client PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}")

ADD_TEST(NAME test_octave_client_multichannel COMMAND ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "simple_client('./native_static', true)" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(test_octave_client_multichannel PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}")


FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/images.txt "")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/groundtruth.txt "")
FOREACH(I RANGE 0 20)
FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/images.txt "${CMAKE_CURRENT_SOURCE_DIR}/data/color.jpg;${CMAKE_CURRENT_SOURCE_DIR}/data/depth.jpg;\n")
FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/groundtruth.txt "153.00,145.00,85.00,193.00\n")
ENDFOREACH()

ADD_TEST(NAME test_native_client COMMAND traxclient -I ${CMAKE_CURRENT_BINARY_DIR}/images.txt -G ${CMAKE_CURRENT_BINARY_DIR}/groundtruth.txt -- ./native_static WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
ADD_TEST(NAME test_native_client_multichannel COMMAND traxclient -I ${CMAKE_CURRENT_BINARY_DIR}/images.txt -G ${CMAKE_CURRENT_BINARY_DIR}/groundtruth.txt -e TRAX_TEST_USE_DEPTH=1 -- ./native_static WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# TODO: test native client


ENDIF()
