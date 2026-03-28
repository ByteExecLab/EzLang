#include "Utils.h"

bool Utils::isNumber(const StackValue& value) {
    return std::holds_alternative<int>(value) || std::holds_alternative<double>(value);
}

bool Utils::bothInt(const StackValue& a, const StackValue& b) {
    return std::holds_alternative<int>(a) && std::holds_alternative<int>(b);
}

double Utils::toDouble(const StackValue& value) {
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

std::string Utils::toDisplayString(const StackValue& value, const bool quoteStrings) {
    return std::visit([quoteStrings](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return "nil";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return quoteStrings ? ('"' + v + '"') : v;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, EzTablePtr>) {
            if (!v) {
                return "<table:null>";
            }

            return "<table entries=" + std::to_string(v->entries.size()) + ">";
        } else if constexpr (std::is_same_v<T, EzUserDataPtr>) {
            if (!v) {
                return "<userdata:null>";
            }

            return "<userdata:" + v->typeName + ">";
        } else {
            return "<unknown>";
        }
    }, value);
}
