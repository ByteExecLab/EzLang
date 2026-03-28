//
// Created by marek on 3/27/2026.
//

#ifndef EZLANG_EZENGINE_H
#define EZLANG_EZENGINE_H
#include <functional>
#include <string>
#include <vector>

#include "Tokenizer.h"

class Interpreter;
class EzRuntime;
struct EzRuntimeLimits;

struct EzCompiledProgram {
    std::string moduleName;
    std::string source;
    std::vector<Token> tokens;
};

using EzNativeRegistrar = std::function<void(Interpreter&)>;
using EzInterpreterSetup = std::function<void(Interpreter&)>;

struct EzEngineConfig {
    std::vector<EzNativeRegistrar> nativeRegistrars;
};

class EzEngine {
public:
    explicit EzEngine(EzEngineConfig config = {});

    [[nodiscard]] EzCompiledProgram compile(std::string source, std::string moduleName = "<memory>") const;
    [[nodiscard]] EzRuntime createRuntime(const EzCompiledProgram& program) const;
    [[nodiscard]] EzRuntime createRuntime(const EzCompiledProgram& program, const EzRuntimeLimits& limits) const;

    void runInterpreted(const EzCompiledProgram& program, const EzInterpreterSetup& extraSetup  = {}) const;

    void runVm(const EzCompiledProgram& program) const;

private:
    EzEngineConfig m_config;
};


#endif //EZLANG_EZENGINE_H
