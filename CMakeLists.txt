cmake_minimum_required(VERSION 3.31)
project(ezlang)

set(CMAKE_CXX_STANDARD 20)

add_executable(ezlang
        main.cpp
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


# Option for Linux static build
option(EZLANG_STATIC "Link ezlang statically (mainly for Linux builds)" OFF)

# Platform-specific linking
if (WIN32)
    # Windows build (MinGW / GCC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Link libstdc++ and libgcc statically so you don't need those DLLs
        target_link_options(ezlang PRIVATE -static -static-libstdc++ -static-libgcc)
    endif()
elseif (UNIX)
    # Linux build
    if (EZLANG_STATIC)
        target_link_options(ezlang PRIVATE -static)
    endif()
endif()
