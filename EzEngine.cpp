//
// Created by marek on 3/27/2026.
//

#include "EzEngine.h"

#include <utility>

#include "compiler.h"
#include "EzError.h"
#include "Interpreter.h"
#include "EzRuntime.h"
#include "vm.h"

EzEngine::EzEngine(EzEngineConfig config): m_config(std::move(config)) {}

EzCompiledProgram EzEngine::compile(std::string source, std::string moduleName) const {
    try {
        Tokenizer tokenizer(source, moduleName);
        auto tokens = tokenizer.tokenize();

        Interpreter::validateBlocks(tokens);

        return EzCompiledProgram{
            .moduleName = std::move(moduleName),
            .source = std::move(source),
            .tokens = std::move(tokens)
        };
    } catch (const EzException&) {
        throw;
    } catch (const std::runtime_error& e) {
        throw EzException(EzError{
            .phase = EzErrorPhase::Parse,
            .message = e.what(),
            .location = EzSourceLocation{moduleName, 0, 0},
            .snippet = {}
        });
    }
}

EzRuntime EzEngine::createRuntime(const EzCompiledProgram& program) const {
    return EzRuntime(program, m_config.nativeRegistrars);
}

EzRuntime EzEngine::createRuntime(const EzCompiledProgram& program, const EzRuntimeLimits& limits) const {
    return EzRuntime(program, m_config.nativeRegistrars, limits);
}

void EzEngine::runInterpreted(const EzCompiledProgram& program,const EzInterpreterSetup& extraSetup) const {
    EzRuntime runtime = createRuntime(program);

    if (extraSetup) {
        extraSetup(runtime.rawInterpreter());
    }

    runtime.initialize();
}

void EzEngine::runVm(const EzCompiledProgram& program) const {
    auto bytecode = compiler::compileToBytecode(program.tokens);
    VM vm(std::move(bytecode));
    vm.run();
}
