#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

#include "EzEngine.h"
#include "Loader.h"
#include "std/StdIo.h"
#include "EzError.h"

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

static const char* phaseToString(const EzErrorPhase phase) {
    switch (phase) {
        case EzErrorPhase::Load:     return "load";
        case EzErrorPhase::Tokenize: return "tokenize";
        case EzErrorPhase::Parse:    return "parse";
        case EzErrorPhase::Runtime:  return "runtime";
    }
    return "unknown";
}

static void printEzError(const EzError& error) {
    std::cerr << "[ERROR]: " << phaseToString(error.phase) << " error encountered!\n";
    std::cerr << "Message      : " << error.message << "\n";

    if (!error.location.module.empty()) {
        std::cerr << "Module       : " << error.location.module << "\n";
    }
    if (error.location.line != 0) {
        std::cerr << "Line         : " << error.location.line << "\n";
    }
    if (error.location.column != 0) {
        std::cerr << "Column       : " << error.location.column << "\n";
    }
    if (!error.snippet.empty()) {
        std::cerr << error.snippet;
    }
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " [--dump-tokens] [--vm] <source-file>\n";
            return 1;
        }

        bool dumpTokens = false;
        bool useVm = false;
        std::string filename;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--dump-tokens") {
                dumpTokens = true;
            } else if (arg == "--vm") {
                useVm = true;
            } else {
                filename = arg;
            }
        }

        if (filename.empty()) {
            std::cerr << "Error: no source file provided.\n";
            return 1;
        }

        std::filesystem::path path = filename;

        EzLoadedModule root = loadModuleFromFilesystem(path);
        std::string source = expandIncludes(root, resolveModuleFromFilesystem);

        EzEngine engine(EzEngineConfig{
            .nativeRegistrars = { registerStdIo }
        });

        EzCompiledProgram program = engine.compile(std::move(source), root.moduleName);

        if (dumpTokens) {
            dumpTokensFn(program.tokens);
            return 0;
        }

        if (useVm) {
            engine.runVm(program);
            return 0;
        }

        engine.runInterpreted(program);
        return 0;
    } catch (const EzException& e) {
        printEzError(e.error());
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR]: unhandled host error\n";
        std::cerr << "Message      : " << e.what() << "\n";
        return 1;
    }
}