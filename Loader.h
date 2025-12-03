//
// Created by marek on 11/23/2025.
//

#ifndef EZLANG_LOADER_H
#define EZLANG_LOADER_H
#include <filesystem>
#include <string>

std::string loadSourceWithIncludes(const std::filesystem::path& path);

#endif //EZLANG_LOADER_H