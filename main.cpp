#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "Interpreter.h"
#include "tokenizer.h"

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct usage is: ez <file>.ex" << std::endl;
        return EXIT_FAILURE;
    }

    std::string source;
    {
        std::ifstream f(argv[1]);
        if (!f.is_open()) {
            std::cerr << "Could not open file: " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }

        std::stringstream buffer;
        buffer << f.rdbuf();
        source = buffer.str();

        f.close();
    }

    tokenizer tokenizer(source);
    auto tokens = tokenizer.tokenize();

    // // DEBUG: print tokens
    // for (const auto& t : tokens) {
    //     std::cout << tokenTypeToString(t.type);
    //     if (std::holds_alternative<int>(t.value)) {
    //         std::cout << "(" << std::get<int>(t.value) << ")\n";
    //     } else if (std::holds_alternative<double>(t.value)) {
    //         std::cout << "(" << std::get<double>(t.value) << ")\n";
    //     } else if (std::holds_alternative<std::string>(t.value)) {
    //         std::cout << "(\"" << std::get<std::string>(t.value) << "\")\n";
    //     }
    //     std::cout << " ";
    // }
    // std::cout << "\n";

    Interpreter::validateBlocks(tokens);

    auto stack = Stack();
    Interpreter(tokens, stack, source).execute();

    return EXIT_SUCCESS;
}