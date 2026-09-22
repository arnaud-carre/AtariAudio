cmake_minimum_required(VERSION 3.20)
project(AtariAudio2Wav LANGUAGES C CXX)

add_executable(AtariAudio2Wav)

target_sources(AtariAudio2Wav
    PRIVATE
        ../../src/external/ice_24.cpp
        ../../src/external/lzh.cpp
        ../../src/external/Musashi/m68k.cpp
        ../../src/external/Musashi/m68kcpu.cpp
        ../../src/external/Musashi/m68kops.cpp
        ../../src/AtariAudioRenderer.cpp
        ../../src/AtariMachine.cpp
        ../../src/Mk68901.cpp
        ../../src/SndhRenderer.cpp
        ../../src/SteDac.cpp
        ../../src/timedb.cpp
        ../../src/ym2149c.cpp
        ../../src/YmRenderer.cpp
        AtariAudio2Wav.cpp
        wavwriter.cpp
)

target_compile_features(AtariAudio2Wav
    PRIVATE
        cxx_std_20
)

target_compile_definitions(AtariAudio2Wav
    PRIVATE
        _CONSOLE
        UNICODE
        _UNICODE
        "$<$<CONFIG:Debug>:_DEBUG>"
        "$<$<CONFIG:Release>:NDEBUG>"
        "$<$<STREQUAL:${CMAKE_CXX_COMPILER_ARCHITECTURE_ID},X86>:WIN32>"
)

if(MSVC)
    target_compile_options(AtariAudio2Wav PRIVATE /W3)
else()
    target_compile_options(AtariAudio2Wav PRIVATE -Wall -Wextra)
endif()
