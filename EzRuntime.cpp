#include "EzRuntime.h"

#include <utility>

namespace {
    EzCallResult unexpectedError(const std::string& moduleName, const std::exception& e) {
        return EzCallResult{
            .ok = false,
            .values = {},
            .error = EzError{
                .phase = EzErrorPhase::Runtime,
                .message = e.what(),
                .location = EzSourceLocation{moduleName, 0, 0},
                .snippet = {}
            }
        };
    }
}

EzRuntime::EzRuntime(const EzCompiledProgram& program,
                     std::vector<EzNativeRegistrar> registrars,
                     EzRuntimeLimits limits)
    : m_program(program),
      m_globalEnv(std::make_shared<EzEnvironment>()),
      m_interpreter(m_program.tokens, Stack{}, m_program.source, m_program.moduleName, m_globalEnv),
      m_limits(std::move(limits)) {
    for (const auto& registrar : registrars) {
        registrar(m_interpreter);
    }

    applyRuntimeLimits();
}

void EzRuntime::initialize() {
    if (m_initialized) {
        return;
    }

    prepareExecution();
    m_interpreter.execute();
    m_initialized = true;
}

EzCallResult EzRuntime::pcallInitialize() {
    try {
        initialize();
        return EzCallResult{
            .ok = true,
            .values = {},
            .error = std::nullopt
        };
    } catch (const EzException& e) {
        return EzCallResult{
            .ok = false,
            .values = {},
            .error = e.error()
        };
    } catch (const std::exception& e) {
        return unexpectedError(m_program.moduleName, e);
    }
}

bool EzRuntime::isInitialized() const {
    return m_initialized;
}

std::vector<StackValue> EzRuntime::callWord(const std::string& name,
                                            const std::vector<StackValue>& args) {
    if (!m_initialized) {
        initialize();
    }

    prepareExecution();
    return m_interpreter.callWord(name, args);
}

EzCallResult EzRuntime::pcallWord(const std::string& name,
                                  const std::vector<StackValue>& args) {
    try {
        return EzCallResult{
            .ok = true,
            .values = callWord(name, args),
            .error = std::nullopt
        };
    } catch (const EzException& e) {
        return EzCallResult{
            .ok = false,
            .values = {},
            .error = e.error()
        };
    } catch (const std::exception& e) {
        return unexpectedError(m_program.moduleName, e);
    }
}

void EzRuntime::registerNativeWord(const std::string& name,
                                   const int arity,
                                   std::function<ControlSignal(Interpreter&)> fn) {
    m_interpreter.registerNativeWord(name, arity, std::move(fn));
}

void EzRuntime::registerHostFunction(const std::string& name, const int arity, EzHostFunction fn) {
    m_interpreter.registerHostFunction(name, arity, std::move(fn));
}

void EzRuntime::pushUserData(std::shared_ptr<void> handle, std::string typeName) {
    m_interpreter.pushUserData(std::move(handle), std::move(typeName));
}

void EzRuntime::setRuntimeLimits(EzRuntimeLimits limits) {
    m_limits = std::move(limits);
    applyRuntimeLimits();
}

bool EzRuntime::hasWord(const std::string& name) const {
    return m_initialized && m_interpreter.hasWord(name);
}

bool EzRuntime::hasGlobal(const std::string& name) const {
    return m_interpreter.hasGlobal(name);
}

StackValue EzRuntime::getGlobal(const std::string& name) const {
    return m_interpreter.getGlobal(name);
}

void EzRuntime::setGlobal(const std::string& name, const StackValue& value, const bool isConst) {
    m_interpreter.setGlobal(name, value, isConst);
}

EzEnvironmentPtr EzRuntime::globalEnvironment() const {
    return m_globalEnv;
}

void EzRuntime::setGlobalEnvironment(EzEnvironmentPtr env) {
    if (m_initialized) {
        throw std::runtime_error("[EZRUNTIME][ERROR]: Cannot replace the global environment after initialization");
    }
    if (!env) {
        throw std::runtime_error("[EZRUNTIME][ERROR]: Global environment cannot be null");
    }
    if (!env->table) {
        env->table = std::make_shared<EzTable>();
    }

    m_globalEnv = std::move(env);
    m_interpreter.setGlobalEnvironment(m_globalEnv);
}

Interpreter& EzRuntime::rawInterpreter() {
    return m_interpreter;
}

const Interpreter& EzRuntime::rawInterpreter() const {
    return m_interpreter;
}

void EzRuntime::applyRuntimeLimits() {
    m_interpreter.setRuntimeLimits(m_limits.instructionBudget, m_limits.maxCallDepth, m_limits.cancelRequested);
}

void EzRuntime::prepareExecution() {
    applyRuntimeLimits();
    m_interpreter.resetRuntimeCounters();
}
