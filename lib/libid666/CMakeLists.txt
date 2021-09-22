cmake_minimum_required(VERSION 3.5)
project(id666)

option(BUILD_DUMP_EXE "Create id666-dump executable to parse a SPC file" OFF)

set(ID666_LIB_SOURCES id666.c)
set(ID666_LIB_HEADERS id666.h)

add_library(id666 STATIC ${ID666_LIB_SOURCES} ${ID666_LIB_HEADERS})
set_property(TARGET id666 PROPERTY POSITION_INDEPENDENT_CODE ON)

if(BUILD_DUMP_EXE)
  set(ID666_DUMP_SOURCES id666-dump.c)

  add_executable(id666-dump ${ID666_DUMP_SOURCES})
  target_link_libraries(id666-dump id666)
endif()

