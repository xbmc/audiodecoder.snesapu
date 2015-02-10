project(audiodecoder.snesapu)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/snes_spc)

set(SNES_SOURCES src/SPCCodec.cpp
                 lib/snes_spc/dsp.cpp
                 lib/snes_spc/SNES_SPC.cpp
                 lib/snes_spc/SNES_SPC_misc.cpp
                 lib/snes_spc/SNES_SPC_state.cpp
                 lib/snes_spc/spc.cpp
                 lib/snes_spc/SPC_DSP.cpp
                 lib/snes_spc/SPC_Filter.cpp)

build_addon(audiodecoder.snesapu SNES DEPLIBS)

include(CPack)
