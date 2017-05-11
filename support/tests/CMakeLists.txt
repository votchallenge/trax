CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

FIND_PROGRAM(OCTAVE octave)

ADD_SUBDIRECTORY(static)
MESSAGE("${CMAKE_CURRENT_SOURCE_DIR}")
ADD_TEST(NAME test_native_static_default COMMAND traxtest -d -- ./native_static WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_python_static_default COMMAND traxtest -d -- python ${CMAKE_CURRENT_SOURCE_DIR}/static/python_static.py WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set_tests_properties(test_native_static_default test_python_static_default PROPERTIES TIMEOUT 10)

IF (OCTAVE)
ADD_TEST(NAME test_octave_static_default COMMAND traxtest -d -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR}/static/ --eval "matlab_static" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_octave_static_default PROPERTIES TIMEOUT 10)
ENDIF()


ADD_TEST(NAME test_native_static_timeout COMMAND traxtest -t 3 -p time_wait=5 -d -- ./native_static WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_python_static_timeout COMMAND traxtest -t 3 -p time_wait=5 -d -- python ${CMAKE_CURRENT_SOURCE_DIR}/static/python_static.py WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set_tests_properties(test_native_static_timeout test_python_static_timeout PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)

IF (OCTAVE)
ADD_TEST(NAME test_octave_static_timeout COMMAND traxtest -d -t 3 -p time_wait=5 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR}/static/ --eval "matlab_static" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_octave_static_timeout PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)
ENDIF()