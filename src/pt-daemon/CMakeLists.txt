
cmake_minimum_required (VERSION 3.2)

project (pt-daemon)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${PT_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

add_executable (pt-daemon "pt-daemon.cpp")

include_directories (
  "${PT_PROJECT_ROOT}/src/pt-daemon-utils/include"
  "${PT_PROJECT_ROOT}/externals/kvr/src")

add_dependencies (pt-daemon pt-daemon-utils)

target_link_libraries (pt-daemon pt-daemon-utils)
