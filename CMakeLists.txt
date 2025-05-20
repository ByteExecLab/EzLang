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
        Common.h)
