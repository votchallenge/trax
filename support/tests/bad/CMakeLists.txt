CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

ADD_EXECUTABLE(badtracker badtracker.cpp)
TARGET_LINK_LIBRARIES(badtracker traxstatic)

ADD_TEST(NAME test_badtracker_abort1 COMMAND traxtest -d -p break_type=sigabrt -p break_time=1 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_badtracker_abort5 COMMAND traxtest -d -p break_type=sigabrt -p break_time=5 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_badtracker_sigsegv1 COMMAND traxtest -d -p break_type=sigsegv -p break_time=1 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_badtracker_sigsegv5 COMMAND traxtest -d -p break_type=sigsegv -p break_time=5 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_badtracker_sigusr1 COMMAND traxtest -d -p break_type=sigusr1 -p break_time=1 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_badtracker_sigusr5 COMMAND traxtest -d -p break_type=sigusr1 -p break_time=5 -- ./badtracker WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set_tests_properties(
	test_badtracker_abort1
	test_badtracker_abort5
	test_badtracker_sigsegv1
	test_badtracker_sigsegv5
	test_badtracker_sigusr1
	test_badtracker_sigusr5
	PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "stopped by signal [0-9]+")

FIND_PROGRAM(MKOCTFILE mkoctfile)

IF ((OCTAVE) AND (MKOCTFILE))

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/badmex.mex COMMAND ${MKOCTFILE}
	-mex -DOCTAVE ${CMAKE_CURRENT_SOURCE_DIR}/badmex.c MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/badmex.c
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMENT "Building badmex.mex")

	ADD_CUSTOM_TARGET(badmex ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/badmex.mex)

ADD_TEST(NAME test_octave_badmex_abort1 COMMAND traxtest -d -t 3 -p break_type=sigabrt -p break_time=1 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_octave_badmex_abort5 COMMAND traxtest -d -t 3 -p break_type=sigabrt -p break_time=5 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_octave_badmex_sigsegv1 COMMAND traxtest -d -t 3 -p break_type=sigsegv -p break_time=1 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_octave_badmex_sigsegv5 COMMAND traxtest -d -t 3 -p break_type=sigsegv -p break_time=5 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_octave_badmex_sigusr1 COMMAND traxtest -d -t 3 -p break_type=sigusr1 -p break_time=1 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
ADD_TEST(NAME test_octave_badmex_sigusr5 COMMAND traxtest -d -t 3 -p break_type=sigusr1 -p break_time=5 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "badtracker" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(
	test_octave_badmex_abort1
	test_octave_badmex_abort5
	test_octave_badmex_sigsegv1
	test_octave_badmex_sigsegv5
	test_octave_badmex_sigusr1
	test_octave_badmex_sigusr5
	PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "stopped by signal [0-9]+")
ENDIF()
