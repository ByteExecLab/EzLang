//
// Created by marek on 11/23/2025.
//

#ifndef EZLANG_LOADER_H
#define EZLANG_LOADER_H
#include <filesystem>
#include <functional>
#include <string>

struct EzLoadedModule {
    std::string moduleName;
    std::string source;
};

using EzImportResolver = std::function<EzLoadedModule(const std::string& fromModule, const std::string& importPath)>;

std::string expandIncludes(const EzLoadedModule& root, const EzImportResolver& resolver);

EzLoadedModule loadModuleFromFilesystem(const std::filesystem::path& path);

EzLoadedModule resolveModuleFromFilesystem(const std::string& fromModule, const std::string& importPath);

#endif //EZLANG_LOADER_H