
cmake_minimum_required (VERSION 3.2)

project (pt-daemon)

add_executable (pt-daemon 
  "src/daemon.cpp"
  )

include_directories (pt-daemon
  ../prof-utils/include
  "${LIBUNWIND_ROOT}/include"
  )