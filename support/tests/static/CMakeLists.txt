CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

ADD_EXECUTABLE(native_static native_static.c)
TARGET_LINK_LIBRARIES(native_static traxstatic)

ADD_EXECUTABLE(native_static_legacy native_static.c)
target_compile_definitions(native_static_legacy PRIVATE -DTRAX_LEGACY_SINGLE)
TARGET_LINK_LIBRARIES(native_static_legacy traxstatic)

ADD_TEST(NAME test_native_static_default COMMAND traxtest -d -- $<TARGET_FILE:native_static> WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_native_static_default PROPERTIES TIMEOUT 10)

IF (PYTHON_EXECUTABLE)
ADD_TEST(NAME test_python_static_default COMMAND traxtest -d -e "${PATHVAR}=${PATHLIST}" -e "PYTHONPATH=${CMAKE_BINARY_DIR}/python/" -- ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python_static.py WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_python_static_default PROPERTIES TIMEOUT 10)
ENDIF()

IF (BUILD_OCTAVE AND OCTAVE)
ADD_TEST(NAME test_octave_static_default COMMAND traxtest -d -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "matlab_static" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_octave_static_default PROPERTIES TIMEOUT 10)
ENDIF()

ADD_TEST(NAME test_native_static_timeout COMMAND traxtest -t 3 -p time_wait=5 -d -- $<TARGET_FILE:native_static> WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_native_static_timeout PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)

IF (PYTHON_EXECUTABLE)
ADD_TEST(NAME test_python_static_timeout COMMAND traxtest -t 3 -p time_wait=5 -d -- ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python_static.py WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_python_static_timeout PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)
ENDIF()

IF (BUILD_OCTAVE AND OCTAVE)
ADD_TEST(NAME test_octave_static_timeout COMMAND traxtest -d -t 3 -p time_wait=5 -- ${OCTAVE} --no-window-system --no-gui --path ${CMAKE_CURRENT_SOURCE_DIR} --eval "matlab_static" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(test_octave_static_timeout PROPERTIES TIMEOUT 10 WILL_FAIL TRUE)
ENDIF()
