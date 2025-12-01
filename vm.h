//
// Created by marek on 11/24/2025.
//

#ifndef EZLANG_VM_H
#define EZLANG_VM_H
#include "bytecode.h"
#include "Stack.h"
#include "Utils.h"

class VM {
public:
    explicit VM(BytecodeProgram program)
    : m_program(std::move(program)) {}

    // Runs until HALT or end of code.
    void run();

    Stack& stack() { return m_stack; }

private:
    void advanceIP();
    
    BytecodeProgram m_program;
    Stack m_stack;
    std::size_t m_ip = 0; // instruction pointer
};

#endif //EZLANG_VM_H