//
// Created by marek on 11/24/2025.
//

#ifndef EZLANG_COMPILER_H
#define EZLANG_COMPILER_H
#include "bytecode.h"
#include "tokenizer.h"


class compiler {
public:
    BytecodeProgram compileToBytecode(const std::vector<Token>& tokens);
};


#endif //EZLANG_COMPILER_H