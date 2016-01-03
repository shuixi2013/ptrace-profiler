
cmake_minimum_required (VERSION 3.2)

project (pt-daemon-utils)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")

# 
# pt-daemon-utils
# 

include_directories (
  "include"
  "include/jansson"
  )

set (JANSSON_SOURCES
  #jansson
  "include/jansson/jansson.h"
  "include/jansson/jansson_config.h"
  "src/jansson/jansson_private.h"
  "src/jansson/dump.c"
  "src/jansson/error.c"
  "src/jansson/hashtable.c"
  "src/jansson/hashtable_seed.c"
  "src/jansson/load.c"
  "src/jansson/memory.c"
  "src/jansson/pack_unpack.c"
  "src/jansson/strbuffer.c"
  "src/jansson/strbuffer.h"
  "src/jansson/strconv.c"
  "src/jansson/utf.h"
  "src/jansson/utf.c"
  "src/jansson/value.c"
  )

source_group (jansson FILES ${JANSSON_SOURCES})

add_library (pt-daemon-utils STATIC
  ${JANSSON_SOURCES}
  # headers
  "include/common.h"
  "include/addr2line.h"
  "include/json_utils.h"
  "include/mmap_manager.h"
  "include/stack_symbolicator.h"
  "include/stack_corkscrew.h"
  # sources
  "src/addr2line.cpp"
  "src/json_utils.cpp"
  "src/mmap_manager.cpp"
  "src/stack_symbolicator.cpp"
  "src/stack_corkscrew.cpp"
  )

# 
# test_map_region
# 

add_executable (test-mmap-manager "tests/test_mmap_manager.cpp")

target_link_libraries (test-mmap-manager pt-daemon-utils)

# 
# test_json_utils
# 

add_executable (test-json-utils "tests/test_json_utils.cpp")

target_link_libraries (test-json-utils pt-daemon-utils)
