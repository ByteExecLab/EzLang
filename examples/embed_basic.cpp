#include <iostream>
#include <string>
#include <vector>

#include "EzEngine.h"
#include "EzError.h"
#include "EzRuntime.h"
#include "Utils.h"

namespace {
    int requireInt(const StackValue& value, const std::string& context) {
        if (!std::holds_alternative<int>(value)) {
            throw std::runtime_error(context + ": expected int result");
        }

        return std::get<int>(value);
    }

    void printCallResult(const std::string& label, const std::vector<StackValue>& values) {
        std::cout << label << " =>";

        if (values.empty()) {
            std::cout << " <no values>\n";
            return;
        }

        for (const auto& value : values) {
            std::cout << ' ' << Utils::toDisplayString(value, false);
        }

        std::cout << '\n';
    }
}

int main() {
    try {
        const std::string source = R"(
            word square 1
                dup *
            end

            word readScore 0
                @score
            end

            word addWithScore 1
                @score host-add
            end

            word failNow 0
                missing-word
            end
        )";

        EzEngine engine;
        const EzCompiledProgram program = engine.compile(source, "embed_basic.ez");

        EzRuntime runtime = engine.createRuntime(program, EzRuntimeLimits{
            .instructionBudget = 1000,
            .maxCallDepth = 32,
            .cancelRequested = {}
        });

        runtime.rawInterpreter().registerHostFunction(
            "host-add",
            2,
            [](const std::vector<StackValue>& args) {
                return std::vector<StackValue>{
                    requireInt(args[0], "host-add arg0") + requireInt(args[1], "host-add arg1")
                };
            }
        );

        runtime.setGlobal("score", 10);
        runtime.initialize();

        const auto square = runtime.callWord("square", {StackValue{5}});
        const auto score = runtime.callWord("readScore");
        const auto sum = runtime.callWord("addWithScore", {StackValue{7}});
        const auto failure = runtime.pcallWord("failNow");

        printCallResult("square(5)", square);
        printCallResult("readScore()", score);
        printCallResult("addWithScore(7)", sum);

        if (!failure.ok && failure.error.has_value()) {
            std::cout << "failNow() => " << failure.error->message << '\n';
        }

        std::cout << "embedded score = " << requireInt(runtime.getGlobal("score"), "score global") << '\n';
        return 0;
    } catch (const EzException& e) {
        const EzError error = e.error();
        std::cerr << "EzLang error: " << error.message << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Host error: " << e.what() << '\n';
        return 1;
    }
}
