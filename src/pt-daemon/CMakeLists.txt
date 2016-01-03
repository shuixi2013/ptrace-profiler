
cmake_minimum_required (VERSION 3.2)

project (pt-daemon)

add_executable (pt-daemon 
  "pt-daemon.cpp"
  )

target_link_libraries (pt-daemon 
  pt-daemon-utils
  )
