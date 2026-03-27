#include "Loader.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include "EzError.h"

namespace {
    [[noreturn]] void throwLoadError(const std::string& message, const std::string& module, size_t line = 0, size_t column = 0, const std::string& snippet = {}) {
        throw EzException(EzError{
            .phase = EzErrorPhase::Load,
            .message = message,
            .location = EzSourceLocation{module, line, column},
            .snippet = snippet
        });
    }

    std::string readFileText(const std::filesystem::path& path) {
        std::ifstream in(path);
        if (!in) {
            throwLoadError("Could not open source file: " + path.string(), path.string());
        }

        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    std::string expandIncludesImpl(const EzLoadedModule& module, const EzImportResolver& resolver, std::unordered_set<std::string>& activeStack) {
        if (!activeStack.insert(module.moduleName).second) {
            throwLoadError("Detected recursive include of file: " + module.moduleName, module.moduleName);
        }

        std::istringstream input(module.source);
        std::ostringstream output;
        std::string line;
        size_t lineNumber = 0;

        while (std::getline(input, line)) {
            ++lineNumber;

            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            if (!trimmed.empty() && trimmed[0] == '#') {
                const auto firstQuote = trimmed.find('"');
                const auto secondQuote = firstQuote == std::string::npos
                    ? std::string::npos
                    : trimmed.find('"', firstQuote + 1);

                if (firstQuote == std::string::npos || secondQuote == std::string::npos) {
                    throwLoadError(
                        "Malformed include directive",
                        module.moduleName,
                        lineNumber,
                        1,
                        "    " + line + "\n    ^\n"
                    );
                }

                const std::string importPath =
                    trimmed.substr(firstQuote + 1, secondQuote - firstQuote - 1);

                EzLoadedModule imported = resolver(module.moduleName, importPath);
                output << expandIncludesImpl(imported, resolver, activeStack);
                output << "\n";
            } else {
                output << line << "\n";
            }
        }

        activeStack.erase(module.moduleName);
        return output.str();
    }
}

std::string expandIncludes(const EzLoadedModule& root, const EzImportResolver& resolver) {
    std::unordered_set<std::string> activeStack;
    return expandIncludesImpl(root, resolver, activeStack);
}

EzLoadedModule loadModuleFromFilesystem(const std::filesystem::path& path) {
    const auto absPath = std::filesystem::absolute(path);
    return EzLoadedModule{
        .moduleName = absPath.string(),
        .source = readFileText(absPath)
    };
}

EzLoadedModule resolveModuleFromFilesystem(const std::string& fromModule, const std::string& importPath) {
    const auto baseDir = std::filesystem::path(fromModule).parent_path();
    const auto absPath = std::filesystem::absolute(baseDir / importPath);

    return EzLoadedModule{
        .moduleName = absPath.string(),
        .source = readFileText(absPath)
    };
}
