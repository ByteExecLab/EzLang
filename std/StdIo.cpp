//
// Created by marek on 12/3/2025.
//

#include "StdIo.h"
#include "../Interpreter.h"

#include <iostream>

void registerStdIo(Interpreter &interpreter) {
    // Raw line -> string | nil
    interpreter.registerNativeWord("sys-read-line", 0,
        [](Interpreter& I) -> ControlSignal {
            if (std::string line; !std::getline(std::cin, line)) {
                I.executePush(StackValue{std::monostate{}});
            }
            else {
                I.executePush(StackValue{line});
            }

            return ControlSignal::None;
        });

    // line -> int | nil
    interpreter.registerNativeWord("sys-read-int", 0,
        [](Interpreter& I) -> ControlSignal {
            std::string line;
            if (!std::getline(std::cin, line)) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            auto trim = [](std::string& s) {
                auto isSpace = [](const unsigned char c) { return std::isspace(c); };
                while (!s.empty() && isSpace(s.front())) s.erase(s.begin());
                while (!s.empty() && isSpace(s.back())) s.pop_back();
            };

            trim(line);

            if (line.empty()) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            try {
                int v = std::stoi(line);
                I.executePush(StackValue{v});
            }
            catch (...) {
                I.executePush(StackValue{std::monostate{}});
            }

            return ControlSignal::None;
        }
    );

    // line -> float | nil
    interpreter.registerNativeWord("sys-read-float", 0,
        [](Interpreter& I) -> ControlSignal {
            std::string line;
            if (!std::getline(std::cin, line)) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            auto trim = [](std::string& s) {
                auto isSpace = [](const unsigned char c){ return std::isspace(c); };
                while (!s.empty() && isSpace(s.front())) s.erase(s.begin());
                while (!s.empty() && isSpace(s.back()))  s.pop_back();
            };

            trim(line);

            if (line.empty()) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            try {
                double v = std::stod(line);
                I.executePush(StackValue{v});
            } catch (...) {
                I.executePush(StackValue{std::monostate{}});
            }

            return ControlSignal::None;
        });
}
