CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

FIND_PROGRAM(OCTAVE octave)

ADD_SUBDIRECTORY(bad)
ADD_SUBDIRECTORY(static)

ADD_TEST(NAME test_nonexisting_tracker COMMAND traxtest -d -- idonotexist WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set_tests_properties(test_nonexisting_tracker PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)

IF (OCTAVE)
ADD_TEST(NAME test_octave_nonexisting COMMAND traxtest -d -t 3 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "idonotexist" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_octave_nonexisting PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
ENDIF()