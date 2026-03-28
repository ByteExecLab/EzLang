#ifndef EZLANG_UTILS_H
#define EZLANG_UTILS_H

#include <iostream>
#include <string>

#include "Common.h"

class Utils {
public:
    static bool isNumber(const StackValue& value);
    static bool bothInt(const StackValue& a, const StackValue& b);
    static double toDouble(const StackValue& value);
    static std::string toDisplayString(const StackValue& value, bool quoteStrings = true);

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

        void operator()(const EzTablePtr& table) const {
            if (!table) {
                std::cout << "<table:null>";
                return;
            }

            std::cout << "<table entries=" << table->entries.size() << ">";
        }

        void operator()(const EzUserDataPtr& data) const {
            if (!data) {
                std::cout << "<userdata:null>";
                return;
            }

            std::cout << "<userdata:" << data->typeName << ">";
        }

        template <typename T>
        void operator()(const T&) const {
            std::cout << "<unknown>";
        }
    };
};

#endif //EZLANG_UTILS_H
