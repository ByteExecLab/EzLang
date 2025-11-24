//
// Created by marek on 11/24/2025.
//

#include "compiler.h"

#include <stdexcept>

BytecodeProgram compiler::compileToBytecode(const std::vector<Token> &tokens) {
    BytecodeProgram program;

    for (const auto & token : tokens) {
        switch (token.type) {
            case TokenType::INT_LITERAL: {
                int v = std::get<int>(token.value);
                program.code.emplace_back(OpCode::PUSH_INT, v);
                break;
            }
            case TokenType::ADD: {
                program.code.emplace_back(OpCode::ADD);
                break;
            }
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
