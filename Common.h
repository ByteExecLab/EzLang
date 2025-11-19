//
// Created by marek on 5/18/2025.
//

#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <variant>

using StackValue = std::variant<std::monostate, int, double, std::string, bool>;


#endif //COMMON_H
