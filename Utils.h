//
// Created by marek on 11/30/2025.
//

#ifndef EZLANG_UTILS_H
#define EZLANG_UTILS_H
#include "Common.h"


class Utils {
public:
    static bool isNumber(const StackValue& value);
    static bool bothInt(const StackValue& a, const StackValue& b);
    static double toDouble(const StackValue& value);
};


#endif //EZLANG_UTILS_H