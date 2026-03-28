cmake_minimum_required(VERSION 3.31)
project(ezlang)

set(CMAKE_CXX_STANDARD 20)

option(EZLANG_STATIC "Link ezlang statically (mainly for Linux builds)" OFF)
option(EZLANG_BUILD_EXAMPLES "Build host embedding examples" ON)
option(EZLANG_BUILD_SMOKE_TESTS "Build embedding smoke tests" ON)

set(EZLANG_CORE_SOURCES
        Tokenizer.cpp
        Tokenizer.h
        Interpreter.cpp
        Interpreter.h
        Stack.cpp
        Stack.h
        Memory.cpp
        Memory.h
        Common.h
        Loader.cpp
        Loader.h
        bytecode.h
        vm.h
        vm.cpp
        compiler.cpp
        compiler.h
        Utils.cpp
        Utils.h
        std/StdIo.cpp
        std/StdIo.h
        std/platform_io.h
        std/platfrom_io.cpp
        EzEngine.cpp
        EzEngine.h
        EzRuntime.cpp
        EzRuntime.h
        EzError.h)

add_library(ezlang_core STATIC ${EZLANG_CORE_SOURCES})
target_include_directories(ezlang_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ezlang main.cpp)
target_link_libraries(ezlang PRIVATE ezlang_core)

if (EZLANG_BUILD_EXAMPLES)
    add_executable(ezlang_embed_basic examples/embed_basic.cpp)
    target_link_libraries(ezlang_embed_basic PRIVATE ezlang_core)
endif ()

if (EZLANG_BUILD_SMOKE_TESTS)
    enable_testing()

    add_executable(ezlang_embed_smoke tests/embed_smoke.cpp)
    target_link_libraries(ezlang_embed_smoke PRIVATE ezlang_core)

    add_test(NAME ezlang_embed_smoke COMMAND ezlang_embed_smoke)
endif ()

function(ezlang_apply_platform_linking target_name)
    if (WIN32)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_link_options(${target_name} PRIVATE -static -static-libstdc++ -static-libgcc)
        endif ()
    elseif (UNIX)
        if (EZLANG_STATIC)
            target_link_options(${target_name} PRIVATE -static)
        endif ()
    endif ()
endfunction()

ezlang_apply_platform_linking(ezlang)

if (TARGET ezlang_embed_basic)
    ezlang_apply_platform_linking(ezlang_embed_basic)
endif ()

if (TARGET ezlang_embed_smoke)
    ezlang_apply_platform_linking(ezlang_embed_smoke)
endif ()
