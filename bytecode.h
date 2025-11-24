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

// Optional: stringify opcodes for debugging / errors
inline const char* toString(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:      return "PUSH_INT";
        case OpCode::PUSH_FLOAT:    return "PUSH_FLOAT";
        case OpCode::PUSH_STRING:   return "PUSH_STRING";
        case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
        case OpCode::ADD:           return "ADD";
        case OpCode::SUB:           return "SUB";
        case OpCode::MUL:           return "MUL";
        case OpCode::DIV:           return "DIV";
        case OpCode::MOD:           return "MOD";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::JUMP:          return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::CALL:          return "CALL";
        case OpCode::RET:           return "RET";
        case OpCode::HLT:           return "HLT";
    }
    return "<unknown-opcode>";
}

#endif //EZLANG_BYTECODE_H