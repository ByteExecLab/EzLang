#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>

#include "Interpreter.h"
#include "tokenizer.h"


void dumpTokensFn(const std::vector<Token>& tokens) {
    std::cout << "==== TOKEN DUMP ====\n\n";
    std::cout << std::left
              << std::setw(4)  << "Idx"
              << std::setw(11) << "Line:Col"
              << std::setw(21) << "Type"
              << "Value\n";

    std::cout << std::setw(4)  << "----"
              << std::setw(11) << "----------"
              << std::setw(21) << "-------------------"
              << "---------------------"
              << "\n";

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];

        std::string pos = std::to_string(t.line) + ":" + std::to_string(t.column);
        std::string type = tokenTypeToString(t.type);

        std::string val;
        if (std::holds_alternative<int>(t.value)) {
            val = std::to_string(std::get<int>(t.value));
        } else if (std::holds_alternative<double>(t.value)) {
            val = std::to_string(std::get<double>(t.value));
        } else if (std::holds_alternative<std::string>(t.value)) {
            const auto& s = std::get<std::string>(t.value);
            if (!s.empty()) val = "\"" + s + "\"";
        }

        std::cout << std::left
                  << std::setw(4)  << i
                  << std::setw(11) << pos
                  << std::setw(21) << type
                  << val << "\n";
    }

    std::cout << std::endl;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [--dump-tokens] <source-file>\n";
        return 1;
    }

    bool dumpTokens = false;
    std::string filename;

    // Very simple arg parsing:
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") {
            dumpTokens = true;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: no source file provided.\n";
        return 1;
    }

    // Read file
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string source = buffer.str();

    // Tokenize
    tokenizer lex(source);
    auto tokens = lex.tokenize();

    if (dumpTokens) {
        dumpTokensFn(tokens);
        return 0;
    }

    // Normal execution path
    auto stack = Stack();
    Interpreter interp(tokens, stack, source);
    interp.execute();

    return 0;
}
