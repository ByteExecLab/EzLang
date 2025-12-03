//
// Created by marek on 12/3/2025.
//

#include "StdIo.h"
#include "platform_io.h"
#include "../Interpreter.h"

#include <iostream>

void registerStdIo(Interpreter &interpreter) {
    // Raw line -> string | nil
    interpreter.registerNativeWord("sys-read-line", 0,
        [](Interpreter& I) -> ControlSignal {
            if (auto lineOpt = platform_io::read_line(); !lineOpt.has_value()) {
                I.executePush(StackValue{std::monostate{}});
            } else {
                I.executePush(StackValue{lineOpt.value()});
            }
            return ControlSignal::None;
        });

    // line -> int | nil
    interpreter.registerNativeWord("sys-read-int", 0,
        [](Interpreter& I) -> ControlSignal {
            const auto lineOpt = platform_io::read_line();
            if (!lineOpt) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            std::string line = *lineOpt;

            auto trim = [](std::string& s) {
                auto isSpace = [](unsigned char c){ return std::isspace(c); };
                while (!s.empty() && isSpace(s.front())) s.erase(s.begin());
                while (!s.empty() && isSpace(s.back()))  s.pop_back();
            };

            trim(line);
            if (line.empty()) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            try {
                int v = std::stoi(line);
                I.executePush(StackValue{v});
            } catch (...) {
                I.executePush(StackValue{std::monostate{}});
            }

            return ControlSignal::None;
        });

    // line -> float | nil
    interpreter.registerNativeWord("sys-read-float", 0,
        [](Interpreter& I) -> ControlSignal {
            const auto lineOpt = platform_io::read_line();
            if (!lineOpt) {
                I.executePush(StackValue{std::monostate{}});
                return ControlSignal::None;
            }

            std::string line = *lineOpt;

            auto trim = [](std::string& s) {
                auto isSpace = [](unsigned char c){ return std::isspace(c); };
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


    // sys-write ( string -- )
    interpreter.registerNativeWord("sys-write", 1,
        [](Interpreter& I) -> ControlSignal {
            auto& st = I.stack();
            if (st.empty()) {
                throw std::runtime_error("[ERROR]: sys-write requires 1 argument");
            }
            StackValue v = st.pop();
            // Convert to string in whatever way your language defines:
            std::string out;

            if (std::holds_alternative<std::string>(v)) {
                out = std::get<std::string>(v);
            } else if (std::holds_alternative<int>(v)) {
                out = std::to_string(std::get<int>(v));
            } else if (std::holds_alternative<double>(v)) {
                out = std::to_string(std::get<double>(v));
            } else if (std::holds_alternative<bool>(v)) {
                out = std::get<bool>(v) ? "true" : "false";
            } else if (std::holds_alternative<std::monostate>(v)) {
                out = "nil"; // or ""
            } else {
                out = "<unsupported>";
            }

            platform_io::write_string(out);
            return ControlSignal::None;
        });

    // sys-write-line ( string -- )
    interpreter.registerNativeWord("sys-write-line", 1,
        [](Interpreter& I) -> ControlSignal {
            auto& st = I.stack();
            if (st.empty()) {
                throw std::runtime_error("[ERROR]: sys-write-line requires 1 argument");
            }
            StackValue v = st.pop();
            std::string out;

            if (std::holds_alternative<std::string>(v)) {
                out = std::get<std::string>(v);
            } else if (std::holds_alternative<int>(v)) {
                out = std::to_string(std::get<int>(v));
            } else if (std::holds_alternative<double>(v)) {
                out = std::to_string(std::get<double>(v));
            } else if (std::holds_alternative<bool>(v)) {
                out = std::get<bool>(v) ? "true" : "false";
            } else if (std::holds_alternative<std::monostate>(v)) {
                out = "nil"; // or ""
            } else {
                out = "<unsupported>";
            }

            platform_io::write_string(out);
            platform_io::write_string("\n");
            return ControlSignal::None;
        });
}
