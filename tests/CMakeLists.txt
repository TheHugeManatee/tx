cmake_minimum_required(VERSION 3.8.2)
# Note : must be included by master CMakeLists.txt
# Tests in static libraries might not get registered, see https://github.com/onqtam/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
# For this reason, and because it is interesting to have individual
# test executables for each library, it is suggested not to put tests directly in the libraries (even though doctest advocates this usage)
# Creating multiple executables is of course not mandatory, and one could use the same executable with various command lines to filter what tests to run.


add_executable(tx_tests testing.cpp)
target_link_libraries(tx_tests tx::tx)

# CMake scripts extensions
target_set_warnings(tx_tests ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_enable_lto(tx_tests optimized) #enable lto if available for non-debug configurations
copy_dlls_for_debug(tx_tests "" "") # Copy dependencies next to the executable (DLLs for example)

# Setup our project as the startup project for Visual so that people don't need to do it manually
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT tx_example)

#set_property(TARGET tx_tests PROPERTY CXX_CLANG_TIDY ON)
#set_property(TARGET tx_tests PROPERTY CXX_CPPLINT ON)
#set_property(TARGET tx_tests PROPERTY CXX_INCLUDE_WHAT_YOU_USE ON)

if(MSVC)
  target_compile_options(tx_tests PRIVATE "/permissive-")
endif()

add_test(
    NAME TX.tests
    COMMAND tx_tests ${TEST_RUNNER_PARAMS}
)


#add_executable(tx_failtests failtests.cpp)
#target_link_libraries(tx_failtests doctest tx::tx)

#add_test(
#    # Use some per-module/project prefix so that it is easier to run only tests for this module
#    NAME TX.failtest
#    COMMAND failtest ${TEST_RUNNER_PARAMS}
#)
# set_tests_properties(
#     BP.failtest
#     PROPERTIES
#         WILL_FAIL TRUE # We expect this test to fail
# )
