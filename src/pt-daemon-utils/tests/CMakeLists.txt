
cmake_minimum_required (VERSION 3.2)

project (pt-daemon-utils-tests)

enable_testing ()

include_directories ("${PT_PROJECT_ROOT}/externals/googletest/googletest/include")

set (PT_DAEMON_UTIL_TEST_LIBRARIES
  pt-daemon-utils
  gtest)

add_executable (test-json-node "./test-json-node.cpp")

target_link_libraries (test-json-node ${PT_DAEMON_UTIL_TEST_LIBRARIES})

add_executable (test-json-populator "./test-json-populator.cpp")

target_link_libraries (test-json-populator ${PT_DAEMON_UTIL_TEST_LIBRARIES})

add_executable (test-mmap-manager "./test-mmap-manager.cpp")

target_link_libraries (test-mmap-manager ${PT_DAEMON_UTIL_TEST_LIBRARIES})

add_executable (test-stack-corkscrew "./test-stack-corkscrew.cpp")

target_link_libraries (test-stack-corkscrew ${PT_DAEMON_UTIL_TEST_LIBRARIES})

add_executable (test-stack-symbolicator "./test-stack-symbolicator.cpp")

target_link_libraries (test-stack-symbolicator ${PT_DAEMON_UTIL_TEST_LIBRARIES})
