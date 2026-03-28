#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "EzEngine.h"
#include "EzRuntime.h"

namespace {
    void require(const bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    int requireInt(const StackValue& value, const std::string& context) {
        require(std::holds_alternative<int>(value), context + ": expected int");
        return std::get<int>(value);
    }

    EzTablePtr requireTable(const StackValue& value, const std::string& context) {
        require(std::holds_alternative<EzTablePtr>(value), context + ": expected table");

        const auto table = std::get<EzTablePtr>(value);
        require(static_cast<bool>(table), context + ": table was null");
        return table;
    }
}

int main() {
    try {
        EzEngine engine;

        const auto basicProgram = engine.compile(R"(
            word square 1
                dup *
            end

            word localIncrement 1
                let x
                x 1 + set x
                x
            end

            word readScore 0
                @score
            end

            word addTwo 2
                host-add
            end

            word makeData 0
                [ 10 20 30 ]
            end

            word boom 0
                missing-word
            end

            word innerNeedsLocal 0
                x
            end

            word outerWithLocal 0
                10 let x
                innerNeedsLocal
            end
        )", "embed_smoke_basic.ez");

        EzRuntime runtime = engine.createRuntime(basicProgram, EzRuntimeLimits{
            .instructionBudget = 1000,
            .maxCallDepth = 32,
            .cancelRequested = {}
        });

        runtime.registerHostFunction(
            "host-add",
            2,
            [](const std::vector<StackValue>& args) {
                return std::vector<StackValue>{
                    requireInt(args[0], "host-add arg0") + requireInt(args[1], "host-add arg1")
                };
            }
        );

        runtime.setGlobal("score", 42);

        require(!runtime.isInitialized(), "runtime should start uninitialized");
        require(!runtime.hasWord("square"), "hasWord should be false before initialize");
        require(runtime.hasGlobal("score"), "host-injected global should exist before initialize");
        require(requireInt(runtime.getGlobal("score"), "score before initialize") == 42,
                "unexpected score before initialize");

        const EzCallResult initResult = runtime.pcallInitialize();
        require(initResult.ok, "pcallInitialize should succeed");
        require(runtime.isInitialized(), "runtime should be initialized after pcallInitialize");

        require(runtime.hasWord("square"), "square should exist after initialize");

        const auto square = runtime.callWord("square", {StackValue{5}});
        require(square.size() == 1, "square should return one value");
        require(requireInt(square[0], "square result") == 25, "square returned wrong value");

        const auto localIncrement = runtime.callWord("localIncrement", {StackValue{5}});
        require(localIncrement.size() == 1, "localIncrement should return one value");
        require(requireInt(localIncrement[0], "localIncrement result") == 6,
                "localIncrement should update and read a local in the same frame");

        const auto score = runtime.callWord("readScore");
        require(score.size() == 1, "readScore should return one value");
        require(requireInt(score[0], "readScore result") == 42, "readScore returned wrong value");

        const auto sum = runtime.callWord("addTwo", {StackValue{7}, StackValue{8}});
        require(sum.size() == 1, "addTwo should return one value");
        require(requireInt(sum[0], "addTwo result") == 15, "addTwo returned wrong value");

        const auto tableValues = runtime.callWord("makeData");
        require(tableValues.size() == 1, "makeData should return one value");
        const auto table = requireTable(tableValues[0], "makeData result");
        require(table->arrayEntryCount() == 3, "makeData table should have 3 array entries");

        const auto failResult = runtime.pcallWord("boom");
        require(!failResult.ok, "boom should fail");
        require(failResult.error.has_value(), "boom should return an error");
        require(failResult.error->phase == EzErrorPhase::Runtime, "boom should return a runtime error");

        const auto localScopeResult = runtime.pcallWord("outerWithLocal");
        require(!localScopeResult.ok, "caller locals should not leak into nested word frames");
        require(localScopeResult.error.has_value(), "outerWithLocal should surface an error");
        require(localScopeResult.error->message.find("Unknown word 'x'") != std::string::npos,
                "nested word should not be able to resolve the caller local");

        const auto loopProgram = engine.compile(R"(
            word spin 0
                1 while 1 do
                end
            end
        )", "embed_smoke_loop.ez");

        EzRuntime limitedRuntime = engine.createRuntime(loopProgram, EzRuntimeLimits{
            .instructionBudget = 1000,
            .maxCallDepth = 32,
            .cancelRequested = {}
        });

        limitedRuntime.setRuntimeLimits(EzRuntimeLimits{
            .instructionBudget = 50,
            .maxCallDepth = 32,
            .cancelRequested = {}
        });

        const auto loopResult = limitedRuntime.pcallWord("spin");
        require(!loopResult.ok, "spin should fail under instruction budget");
        require(loopResult.error.has_value(), "spin should return an error");
        require(loopResult.error->message.find("Instruction budget exceeded") != std::string::npos,
                "spin should fail because of the instruction budget");

        const auto recurseProgram = engine.compile(R"(
            word recur 0
                recur
            end
        )", "embed_smoke_recur.ez");

        EzRuntime recursiveRuntime = engine.createRuntime(recurseProgram, EzRuntimeLimits{
            .instructionBudget = 1000,
            .maxCallDepth = 8,
            .cancelRequested = {}
        });

        const auto recurResult = recursiveRuntime.pcallWord("recur");
        require(!recurResult.ok, "recur should fail under max call depth");
        require(recurResult.error.has_value(), "recur should return an error");
        require(recurResult.error->message.find("Maximum call depth exceeded") != std::string::npos,
                "recur should fail because of max call depth");

        const auto startupFailProgram = engine.compile(R"(
            missing-at-startup
        )", "embed_smoke_startup_fail.ez");

        EzRuntime startupFailRuntime = engine.createRuntime(startupFailProgram);
        const auto startupFailResult = startupFailRuntime.pcallInitialize();
        require(!startupFailResult.ok, "startup failure should be reported through pcallInitialize");
        require(startupFailResult.error.has_value(), "pcallInitialize failure should include an error");
        require(startupFailResult.error->message.find("Unknown word") != std::string::npos,
                "startup failure should preserve the underlying runtime message");
        require(!startupFailRuntime.isInitialized(), "failed initialization should not mark runtime initialized");

        std::cout << "embed smoke ok\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "embed smoke failed: " << e.what() << '\n';
        return 1;
    }
}
