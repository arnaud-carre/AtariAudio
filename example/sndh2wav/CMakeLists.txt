cmake_minimum_required(VERSION 4.0)
project(sndh2wav LANGUAGES C CXX)

add_executable(sndh2wav)

target_sources(sndh2wav
    PRIVATE
        ../../src/AtariMachine.cpp
        ../../src/external/ice_24.c
        ../../src/external/Musashi/m68kcpu.c
        ../../src/external/Musashi/m68kops.c
        ../../src/Mk68901.cpp
        ../../src/SndhFile.cpp
        ../../src/SteDac.cpp
        ../../src/timedb.cpp
        ../../src/ym2149c.cpp
        sndh2wav.cpp
        wavwriter.cpp
)

target_compile_features(sndh2wav
    PRIVATE
        cxx_std_20
)

target_compile_definitions(sndh2wav
    PRIVATE
        _CONSOLE
        UNICODE
        _UNICODE
        "$<$<CONFIG:Debug>:_DEBUG>"
        "$<$<CONFIG:Release>:NDEBUG>"
        "$<$<STREQUAL:${CMAKE_CXX_COMPILER_ARCHITECTURE_ID},X86>:WIN32>"
)

target_compile_options(sndh2wav
    PRIVATE
        /W3
)

