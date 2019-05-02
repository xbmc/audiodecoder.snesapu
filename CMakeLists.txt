project(audiodecoder.snesapu)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/lib/snes_spc)

add_subdirectory(lib/snes_spc)

set(SNES_SOURCES src/SPCCodec.cpp)

set(DEPLIBS snes_spc)

build_addon(audiodecoder.snesapu SNES DEPLIBS)

include(CPack)
