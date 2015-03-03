project(snes_spc)

cmake_minimum_required(VERSION 2.6)

enable_language(CXX)

set(SOURCES dsp.cpp
            SNES_SPC.cpp
            SNES_SPC_misc.cpp
            SNES_SPC_state.cpp
            spc.cpp
            SPC_DSP.cpp
            SPC_Filter.cpp)

if(NOT WIN32)
  add_options(ALL_LANGUAGES ALL_BUILDS "-fPIC")
endif()

add_library(snes_spc STATIC ${SOURCES})
