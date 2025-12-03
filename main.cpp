#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <filesystem>

#include "compiler.h"
#include "Interpreter.h"
#include "tokenizer.h"
#include "Loader.h"
#include "vm.h"

// Std
#include "std/StdIo.h"

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

        std::string pos  = std::to_string(t.line) + ":" + std::to_string(t.column);
        std::string type = tokenTypeToString(t.type);

        std::string val;
        if (std::holds_alternative<int>(t.value)) {
            val = std::to_string(std::get<int>(t.value));
        } else if (std::holds_alternative<double>(t.value)) {
            val = std::to_string(std::get<double>(t.value));
        } else if (std::holds_alternative<std::string>(t.value)) {
            const auto& s = std::get<std::string>(t.value);
            if (!s.empty()) val = "\"" + s + "\"";
        } else if (std::holds_alternative<bool>(t.value)) {
            val = std::get<bool>(t.value) ? "true" : "false";
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
        std::cerr << "Usage: " << argv[0] << " [--dump-tokens] [--vm] <source-file>\n";
        return 1;
    }

    bool dumpTokens = false;
    bool useVm = false;

    std::string filename;

    // Very simple arg parsing:
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") {
            dumpTokens = true;
        }
        else if (arg == "--vm") {
            useVm = true;
        }
        else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: no source file provided.\n";
        return 1;
    }

    // Load + expand includes
    std::filesystem::path path = filename;
    std::string source = loadSourceWithIncludes(path);

    // Tokenize
    tokenizer lex(source);
    auto tokens = lex.tokenize();

    if (dumpTokens) {
        dumpTokensFn(tokens);
        return 0;
    }

    if (useVm) {
        compiler compiler{};
        auto prog = compiler.compileToBytecode(tokens);

        VM vm(std::move(prog));
        vm.run();
        return 0;
    }

    // Normal execution path
    Stack stack;
    Interpreter interp(tokens, stack, source);

    // Register
    registerStdIo(interp);

    interp.execute();

    return 0;
}
