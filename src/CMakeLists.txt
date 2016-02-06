
cmake_minimum_required (VERSION 3.2)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (PT_CXX_FLAGS "-std=c++11 -stdlib=libc++ -Wall -Wextra -Werror")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set (PT_CXX_FLAGS "-std=c++11 -stdlib=libc++ -Wall -Wextra -Werror")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set (PT_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS=1")
endif()

add_subdirectory (pt-daemon)

add_subdirectory (pt-daemon-utils)
