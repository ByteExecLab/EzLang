//
// Created by marek on 11/30/2025.
//

#include "Utils.h"

bool Utils::isNumber(const StackValue &value) {
    return std::holds_alternative<int>(value) || std::holds_alternative<double>(value);
}

bool Utils::bothInt(const StackValue& a, const StackValue& b) {
    return std::holds_alternative<int>(a) && std::holds_alternative<int>(b);
}

double Utils::toDouble(const StackValue &value) {
    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value);
    }

    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }

    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? 1.0 : 0.0;
    }

    throw std::runtime_error("[ERROR]: Expected numeric value, got non-numeric type");
}
