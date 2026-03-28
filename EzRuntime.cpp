#include "EzRuntime.h"

#include <utility>

EzRuntime::EzRuntime(const EzCompiledProgram& program,
                     std::vector<EzNativeRegistrar> registrars,
                     EzRuntimeLimits limits)
    : m_program(program),
      m_interpreter(m_program.tokens, Stack{}, m_program.source, m_program.moduleName),
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
        return EzCallResult{
            .ok = false,
            .values = {},
            .error = EzError{
                .phase = EzErrorPhase::Runtime,
                .message = e.what(),
                .location = EzSourceLocation{m_program.moduleName, 0, 0},
                .snippet = {}
            }
        };
    }
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
