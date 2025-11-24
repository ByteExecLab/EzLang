//
// Created by marek on 11/24/2025.
//

#ifndef EZLANG_BYTECODE_H
#define EZLANG_BYTECODE_H
#include <cstdint>
#include <vector>

#include "Common.h"

// Low-level instructions the VM understands
enum class OpCode {
    // Stack / literals
    PUSH_INT,
    PUSH_FLOAT,
    PUSH_STRING,
    PUSH_BOOL,

    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    // Print
    PRINT,

    // Control Flow
    JUMP,
    JUMP_IF_FALSE,

    // Call / return
    CALL,
    RET,

    // End program
    HLT,
};

// Simple instruction format.
// For now: 1 opcode + optional integer/string indices.
struct Instruction {
    OpCode op;

    // A generic integer field (offset, constant index, etc.)
    int32_t argInt = 0;

    // Optional index into constant table (for strings later)
    int32_t argIndex = -1;

    Instruction() = default;
    explicit Instruction(OpCode o) : op(o) {}
    Instruction(OpCode o, int32_t a) : op(o), argInt(a) {}
    Instruction(OpCode o, int32_t a, int32_t idx) : op(o), argInt(a), argIndex(idx) {}

};

struct BytecodeProgram {
    std::vector<Instruction> code;
    std::vector<StackValue>  constants;  // for strings/floats later
};

#endif //EZLANG_BYTECODE_H