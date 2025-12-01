//
// Created by marek on 11/30/2025.
//

#ifndef EZLANG_UTILS_H
#define EZLANG_UTILS_H
#include "Common.h"
#include <iostream>


class Utils {
public:
    static bool isNumber(const StackValue& value);
    static bool bothInt(const StackValue& a, const StackValue& b);
    static double toDouble(const StackValue& value);

public:
    struct PrintVisitor {
        void operator()(const std::monostate&) const {
            std::cout << "nil";
        }

        void operator()(const int v) const {
            std::cout << v;
        }

        void operator()(const double v) const {
            std::cout << v;
        }

        void operator()(const std::string& s) const {
            std::cout << '"' << s << '"';
        }

        void operator()(const bool b) const {
            std::cout << (b ? "true" : "false");
        }

        void operator()(const ArrayValue& arr) const {
            std::cout << "<Array len=" << arr.elements.size() << ">";
        }

        void operator()(const StructValue& obj) const {
            std::cout << "<Struct fields=" << obj.fields.size() << ">";
        }

        template <typename T>
        void operator()(const T&) const {
            std::cout << "<unknown>";
        }
    };
};


#endif //EZLANG_UTILS_H