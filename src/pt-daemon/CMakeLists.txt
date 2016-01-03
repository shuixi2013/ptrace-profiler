
cmake_minimum_required (VERSION 3.2)

project (pt-daemon)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")

add_executable (pt-daemon 
  "pt-daemon.cpp"
  )

include_directories (pt-daemon
  ../pt-daemon-utils/include
  )

target_link_libraries (pt-daemon 
  pt-daemon-utils
  )
