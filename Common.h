//
// Created by marek on 5/18/2025.
//

#ifndef COMMON_H
#define COMMON_H
#include <memory>
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

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
    std::vector<std::shared_ptr<StackValue>> elements;

    ArrayValue() = default;
    explicit ArrayValue(std::vector<StackValue> src);
};

struct StructValue {
    std::unordered_map<std::string, std::shared_ptr<StackValue>> fields;
};

inline ArrayValue::ArrayValue(std::vector<StackValue> src) {
    elements.reserve(src.size());
    for (auto &v : src) {
        elements.push_back(std::make_shared<StackValue>(std::move(v)));
    }
}


#endif //COMMON_H
