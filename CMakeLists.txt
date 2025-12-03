cmake_minimum_required(VERSION 3.31)
project(ezlang)

set(CMAKE_CXX_STANDARD 20)

add_executable(ezlang main.cpp
        tokenizer.cpp
        tokenizer.h
        Interpreter.cpp
        Interpreter.h
        Stack.cpp
        Stack.h
        Memory.cpp
        Memory.h
        Common.h
        main.cpp
        main.cpp
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
        std/StdIo.h)


option(EZLANG_STATIC "Link ezlang statically" OFF)

if(EZLANG_STATIC)
    target_link_options(ezlang PRIVATE -static)
endif()
