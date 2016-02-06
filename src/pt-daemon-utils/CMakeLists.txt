
cmake_minimum_required (VERSION 3.2)

project (pt-daemon-utils)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

set (PT_DAEMON_UTILS_INCLUDE_DIRS
  "include"
  "${PT_PROJECT_ROOT}/externals/kvr/src")

set (PT_DAEMON_UTILS_INCLUDES
  "include/addr2line.h"
  "include/json-populator.h"
  #"include/json-utils.h"
  "include/mmap-manager.h"
  "include/stack-corkscrew.h"
  "include/stack-symbolicator.h"
  )

set (PT_DAEMON_UTILS_SRCS
  "src/addr2line.cpp"
  "src/json-populator.cpp"
  #"src/json-utils.cpp"
  "src/mmap-manager.cpp"
  "src/stack-corkscrew.cpp"
  "src/stack-symbolicator.cpp"
  )

include_directories (
  "${PT_DAEMON_UTILS_INCLUDE_DIRS}"
  )

add_library (pt-daemon-utils STATIC
  ${PT_DAEMON_UTILS_INCLUDES}
  ${PT_DAEMON_UTILS_SRCS}
  )

add_dependencies (pt-daemon-utils kvr)

target_link_libraries (pt-daemon-utils kvr)

#
# test-mmap-manager
#

add_executable (test-mmap-manager "tests/test-mmap-manager.cpp")

target_link_libraries (test-mmap-manager pt-daemon-utils)
