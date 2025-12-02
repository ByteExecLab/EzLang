//
// Created by marek on 11/30/2025.
//

#ifndef EZLANG_UTILS_H
#define EZLANG_UTILS_H
#include "Common.h"
#include <iostream>


class Utils {
public:
    /**
     * Checks if the provided StackValue is a numeric type (either int or double).
     *
     * @param value The StackValue to check.
     * @return true if the StackValue is an int or double, false otherwise.
     */
    static bool isNumber(const StackValue& value);

    /**
     * Checks if both provided StackValue instances are of type int.
     *
     * @param a The first StackValue to check.
     * @param b The second StackValue to check.
     * @return true if both StackValue instances are of type int, false otherwise.
     */
    static bool bothInt(const StackValue& a, const StackValue& b);

    /**
     * Converts the provided StackValue to a double, if possible.
     * Supports conversion from int, double, and bool types.
     * If the value is of type int, it is converted to its double representation.
     * If the value is of type double, it is returned as is.
     * If the value is of type bool, it is converted to 1.0 for true and 0.0 for false.
     * Throws an error if the value is not numeric or not convertible to double.
     *
     * @param value The StackValue to convert.
     * @return The numeric value of the StackValue as a double.
     * @throws std::runtime_error if the StackValue is not a numeric or convertible type.
     */
    static double toDouble(const StackValue& value);

    /**
     * A visitor class responsible for printing the representation of
     * visited elements. This class is typically used in conjunction
     * with the Visitor design pattern to handle printing logic for
     * various types within a structured hierarchy.
     */
    struct PrintVisitor {
        /**
         * Handles the case where the visited value is of type std::monostate.
         * Outputs "nil" as the representation for the std::monostate type.
         *
         * @param unused The std::monostate value being visited.
         */
        void operator()(const std::monostate&) const {
            std::cout << "nil";
        }

        /**
         * Prints an integer value to the standard output.
         *
         * @param v The integer value to print.
         */
        void operator()(const int v) const {
            std::cout << v;
        }

        /**
         * Prints a double value to the standard output.
         *
         * @param v The double value to print.
         */
        void operator()(const double v) const {
            std::cout << v;
        }

        /**
         * Prints a string value enclosed in double quotes.
         *
         * @param s The string value to print.
         */
        void operator()(const std::string& s) const {
            std::cout << '"' << s << '"';
        }


        /**
         * Prints the boolean value as "true" or "false".
         *
         * @param b The boolean value to print.
         */
        void operator()(const bool b) const {
            std::cout << (b ? "true" : "false");
        }

        /**
         * Prints a representation of an ArrayValue object, including the size of its elements.
         *
         * @param arr The ArrayValue instance to be printed.
         */
        void operator()(const ArrayValue& arr) const {
            std::cout << "<Array len=" << arr.elements.size() << ">";
        }

        /**
         * Prints a representation of a StructValue object, including the number of its fields.
         *
         * @param obj The StructValue instance to be printed.
         */
        void operator()(const StructValue& obj) const {
            std::cout << "<Struct fields=" << obj.fields.size() << ">";
        }


        /**
         * Overloads an operator to handle cases for unknown or unsupported types.
         * Outputs a default representation for such types as "<unknown>".
         *
         * @param value The value of an unspecified or unsupported type to be handled.
         */
        template <typename T>
        void operator()(const T&) const {
            std::cout << "<unknown>";
        }
    };
};


#endif //EZLANG_UTILS_H