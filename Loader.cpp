//
// Created by marek on 11/23/2025.
//

#include "Loader.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

namespace {

    std::string loadSourceWithIncludesImpl(const std::filesystem::path& path,
                                           std::unordered_set<std::string>& seen) {
        const auto absPath = std::filesystem::absolute(path);
        const std::string key = absPath.string();

        if (seen.contains(key)) {
            std::cerr << "[ERROR]: Detected recursive include of file: " << key << "\n";
            std::exit(EXIT_FAILURE);
        }
        seen.insert(key);

        std::ifstream in(absPath);
        if (!in) {
            std::cerr << "[ERROR]: Could not open source file: " << key << "\n";
            std::exit(EXIT_FAILURE);
        }

        std::ostringstream out;
        std::string line;
        const auto baseDir = absPath.parent_path();

        while (std::getline(in, line)) {
            // Make a trimmed copy for directive detection
            std::string trimmed = line;
            // strip leading spaces/tabs
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            if (!trimmed.empty() && trimmed[0] == '#') {
                // We expect: # "path/to/file.ez"
                auto firstQuote = trimmed.find('"');
                auto secondQuote = std::string::npos;
                if (firstQuote != std::string::npos) {
                    secondQuote = trimmed.find('"', firstQuote + 1);
                }

                if (firstQuote == std::string::npos || secondQuote == std::string::npos) {
                    std::cerr << "[ERROR]: Malformed include directive: " << line << "\n";
                    std::exit(EXIT_FAILURE);
                }

                const std::string includePathStr =
                    trimmed.substr(firstQuote + 1, secondQuote - firstQuote - 1);

                std::filesystem::path includePath = baseDir / includePathStr;

                // Recursively load included file
                out << loadSourceWithIncludesImpl(includePath, seen);
                // Keep a newline boundary between files to preserve line structure
                out << "\n";
            } else {
                out << line << "\n";
            }
        }

        return out.str();
    }
} // Anonymous namespace

std::string loadSourceWithIncludes(const std::filesystem::path& path) {
    std::unordered_set<std::string> seen;
    return loadSourceWithIncludesImpl(path, seen);
}
