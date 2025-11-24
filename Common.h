//
// Created by marek on 5/18/2025.
//

#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <variant>
#include <unordered_map>

struct ArrayValue;
struct StructValue;

using StackValue = std::variant<
    std::monostate,
    int,
    double,
    std::string,
    bool,
    ArrayValue,
    StructValue
>;

struct ArrayValue {
    std::vector<StackValue> elements;
};

struct StructValue {
    std::unordered_map<std::string, StackValue> fields;
};

#endif //COMMON_H
