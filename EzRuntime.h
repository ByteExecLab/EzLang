#ifndef EZLANG_EZRUNTIME_H
#define EZLANG_EZRUNTIME_H

#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "EzEngine.h"
#include "EzError.h"
#include "Interpreter.h"

struct EzRuntimeLimits {
    size_t instructionBudget = std::numeric_limits<size_t>::max();
    size_t maxCallDepth = 256;
    std::function<bool()> cancelRequested;
};

struct EzCallResult {
    bool ok = false;
    std::vector<StackValue> values;
    std::optional<EzError> error;
};

class EzRuntime {
public:
    EzRuntime(const EzCompiledProgram& program,
              std::vector<EzNativeRegistrar> registrars = {},
              EzRuntimeLimits limits = {});

    void initialize();
    [[nodiscard]] bool isInitialized() const;

    std::vector<StackValue> callWord(const std::string& name,
                                     const std::vector<StackValue>& args = {});
    EzCallResult pcallWord(const std::string& name,
                           const std::vector<StackValue>& args = {});

    [[nodiscard]] bool hasWord(const std::string& name) const;
    [[nodiscard]] bool hasGlobal(const std::string& name) const;
    [[nodiscard]] StackValue getGlobal(const std::string& name) const;
    void setGlobal(const std::string& name, const StackValue& value, bool isConst = false);

    Interpreter& rawInterpreter();
    const Interpreter& rawInterpreter() const;

private:
    void applyRuntimeLimits();
    void prepareExecution();

    EzCompiledProgram m_program;
    Interpreter m_interpreter;
    bool m_initialized = false;
    EzRuntimeLimits m_limits;
};

#endif //EZLANG_EZRUNTIME_H
