//
// Created by marek on 11/24/2025.
//

#include "compiler.h"

#include <stdexcept>

BytecodeProgram compiler::compileToBytecode(const std::vector<Token> &tokens) {
    BytecodeProgram program;

    for (const auto & token : tokens) {
        switch (token.type) {
            // ---------------- Literals -----------------------
            case TokenType::INT_LITERAL: {
                int v = std::get<int>(token.value);
                program.code.emplace_back(OpCode::PUSH_INT, v);
                break;
            }
            case TokenType::FLOAT_LITERAL: {
                double v = std::get<double>(token.value);
                program.constants.emplace_back(v);
                const auto idx = static_cast<int32_t>(program.constants.size() - 1);

                program.code.emplace_back(OpCode::PUSH_FLOAT, 0, idx);

                break;
            }
            case TokenType::BOOL_LITERAL: {
                int v = std::get<bool>(token.value);
                program.code.emplace_back(OpCode::PUSH_BOOL, v);
                break;
            }
            case TokenType::STR_LITERAL: {
                const auto& v = std::get<std::string>(token.value);

                // Put string into a constant table
                program.constants.emplace_back(v);
                const auto idx = static_cast<int32_t>(program.constants.size() - 1);

                // encode only the constant index in the instruction
                program.code.emplace_back(OpCode::PUSH_STRING, /*argInt*/ 0, /*argIndex*/ idx);
                break;
            }
            // ------------------- Maths -----------------------
            case TokenType::ADD: {
                program.code.emplace_back(OpCode::ADD);
                break;
            }
            case TokenType::SUB: {
                program.code.emplace_back(OpCode::SUB);
                break;
            }
            case TokenType::MUL: {
                program.code.emplace_back(OpCode::MUL);
                break;
            }
            case TokenType::DIV: {
                program.code.emplace_back(OpCode::DIV);
                break;
            }
            case TokenType::MOD: {
                program.code.emplace_back(OpCode::MOD);
                break;
            }
            // --------------------- PRINT ---------------------
            case TokenType::PRINT: {
                program.code.emplace_back(OpCode::PRINT);
                break;
            }
            default: {
                throw std::runtime_error(
                    "compileToBytecode: unsupported token " + tokenTypeToString(token.type)
                );
            }
        }
    }

    program.code.emplace_back(OpCode::HLT);
    return program;
}
