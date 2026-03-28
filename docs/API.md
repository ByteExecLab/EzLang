# EzLang API Reference

This document describes the current host-facing C++ API for embedding EzLang.

For a small end-to-end host example, see [examples/embed_basic.cpp](/examples/embed_basic.cpp).
For a stricter regression-style embedding check, see [tests/embed_smoke.cpp](/tests/embed_smoke.cpp).

The intended flow is:

1. Create an `EzEngine`
2. Compile source into an `EzCompiledProgram`
3. Create an `EzRuntime`
4. Register host functions or low-level native words
5. Inject globals if needed
6. Initialize once
7. Call words repeatedly

Each `EzRuntime` gets its own global environment by default.
If a host wants shared global state, it can explicitly install the same `EzEnvironment` into multiple runtimes before initialization.

In the current API, prefer `EzRuntime` methods first.
Use `rawInterpreter()` only when you need lower-level access that has not been promoted yet.

## Core Types

### `EzCompiledProgram`

Declared in [EzEngine.h](/EzEngine.h).

```cpp
struct EzCompiledProgram {
    std::string moduleName;
    std::string source;
    std::vector<Token> tokens;
};
```

This is the result of `EzEngine::compile(...)`.

- `moduleName`: the source name used in diagnostics
- `source`: the full source text
- `tokens`: the tokenized and block-validated program

In the current implementation, this is a lightweight compiled chunk for the interpreter path.

### `StackValue`

Declared in [Common.h](/Common.h).

```cpp
using StackValue = std::variant<
    std::monostate,
    int,
    double,
    std::string,
    bool,
    EzTablePtr,
    EzUserDataPtr
>;
```

Current host-visible value kinds:

- `std::monostate`: EzLang `nil`
- `int`
- `double`
- `std::string`
- `bool`
- `EzTablePtr`: shared table value used for both arrays and structs
- `EzUserDataPtr`: opaque host-owned userdata

Notes:

- Arrays and structs are now one runtime type internally: `EzTable`
- Tables are shared through `std::shared_ptr`
- Userdata is also shared through `std::shared_ptr`

### `EzTable`

Declared in [Common.h](/Common.h).

```cpp
struct EzTable {
    std::unordered_map<EzTableKey, std::shared_ptr<StackValue>, EzTableKeyHash> entries;

    EzTable() = default;
    explicit EzTable(std::vector<StackValue> src);

    [[nodiscard]] size_t arrayEntryCount() const;
};
```

Current language mapping:

- `[ ... ]` produces a table with integer keys `0..n-1`
- `{ "key" value ... }` produces a table with string keys

### `EzUserData`

Declared in [Common.h](/Common.h).

```cpp
struct EzUserData {
    std::shared_ptr<void> handle;
    std::string typeName;
};
```

Use userdata for opaque host-owned values like engine objects, handles, or file/socket wrappers.

### `EzEnvironment`

Declared in [Common.h](/Common.h).

```cpp
struct EzEnvironment {
    EzTablePtr table = std::make_shared<EzTable>();
    std::unordered_set<std::string> constNames;
    EzEnvironmentPtr parent;
};
```

`EzEnvironment` is the runtime global-binding container.

Current behavior:

- each `EzRuntime` owns one global environment by default
- `var` and `const` create bindings in that environment
- `@name` reads through the environment chain
- `value ! name` updates an existing binding through the environment chain
- `setGlobal(...)` from the host writes into the runtime's current environment
- `parent` exists as groundwork for future environment chaining/module design

## Errors

Declared in [EzError.h](/EzError.h).

### `EzErrorPhase`

```cpp
enum class EzErrorPhase {
    Load,
    Tokenize,
    Parse,
    Runtime
};
```

### `EzSourceLocation`

```cpp
struct EzSourceLocation {
    std::string module = "<unknown>";
    size_t line = 0;
    size_t column = 0;
};
```

### `EzError`

```cpp
struct EzError {
    EzErrorPhase phase;
    std::string message;
    EzSourceLocation location;
    std::string snippet;
};
```

### `EzException`

```cpp
class EzException : public std::runtime_error {
public:
    explicit EzException(EzError error);
    [[nodiscard]] EzError error() const;
};
```

Use `EzException` for throwing paths.
Use `EzRuntime::pcallWord(...)` for protected host calls that return errors as data instead of throwing.

## Engine API

Declared in [EzEngine.h](/EzEngine.h).

### `EzEngineConfig`

```cpp
using EzNativeRegistrar = std::function<void(Interpreter&)>;

struct EzEngineConfig {
    std::vector<EzNativeRegistrar> nativeRegistrars;
};
```

`nativeRegistrars` are applied to each runtime created by the engine.

Typical use:

```cpp
EzEngine engine(EzEngineConfig{
    .nativeRegistrars = { registerStdIo }
});
```

### `EzEngine`

```cpp
class EzEngine {
public:
    explicit EzEngine(EzEngineConfig config = {});

    [[nodiscard]] EzCompiledProgram compile(std::string source,
                                            std::string moduleName = "<memory>") const;

    [[nodiscard]] EzRuntime createRuntime(const EzCompiledProgram& program) const;
    [[nodiscard]] EzRuntime createRuntime(const EzCompiledProgram& program,
                                          const EzRuntimeLimits& limits) const;

    void runInterpreted(const EzCompiledProgram& program,
                        const EzInterpreterSetup& extraSetup = {}) const;

    void runVm(const EzCompiledProgram& program) const;
};
```

#### `compile(...)`

Compiles a source string into an `EzCompiledProgram`.

Behavior:

- tokenizes the source
- validates block structure
- preserves `moduleName` for diagnostics

Throws:

- `EzException` for tokenize failures
- `EzException` wrapping parse/block-validation failures

#### `createRuntime(...)`

Creates a reusable runtime from a compiled program.

Use this for embedded scenarios.

#### `runInterpreted(...)`

Convenience path for one-shot interpreted execution.

Behavior:

- creates a runtime
- applies engine-level registrars
- applies optional `extraSetup`
- initializes the runtime once

#### `runVm(...)`

Compiles the token stream to bytecode and runs the VM.

This is currently a one-shot execution path, not the main embedding API.

## Runtime API

Declared in [EzRuntime.h](/EzRuntime.h).

### `EzRuntimeLimits`

```cpp
struct EzRuntimeLimits {
    size_t instructionBudget = std::numeric_limits<size_t>::max();
    size_t maxCallDepth = 256;
    std::function<bool()> cancelRequested;
};
```

Fields:

- `instructionBudget`: maximum executed tokens/instructions per initialize/call cycle
- `maxCallDepth`: maximum nested user-word depth
- `cancelRequested`: optional host callback; return `true` to cancel execution

### `EzCallResult`

```cpp
struct EzCallResult {
    bool ok = false;
    std::vector<StackValue> values;
    std::optional<EzError> error;
};
```

When `ok` is `true`, `values` contains the returned stack outputs.
When `ok` is `false`, `error` contains failure details.

### `EzRuntime`

```cpp
class EzRuntime {
public:
    EzRuntime(const EzCompiledProgram& program,
              std::vector<EzNativeRegistrar> registrars = {},
              EzRuntimeLimits limits = {});

    void initialize();
    EzCallResult pcallInitialize();
    [[nodiscard]] bool isInitialized() const;

    std::vector<StackValue> callWord(const std::string& name,
                                     const std::vector<StackValue>& args = {});
    EzCallResult pcallWord(const std::string& name,
                           const std::vector<StackValue>& args = {});

    void registerNativeWord(const std::string& name,
                            int arity,
                            std::function<ControlSignal(Interpreter&)> fn);
    void registerHostFunction(const std::string& name, int arity, EzHostFunction fn);
    void pushUserData(std::shared_ptr<void> handle, std::string typeName);
    void setRuntimeLimits(EzRuntimeLimits limits);

    [[nodiscard]] bool hasWord(const std::string& name) const;
    [[nodiscard]] bool hasGlobal(const std::string& name) const;
    [[nodiscard]] StackValue getGlobal(const std::string& name) const;
    void setGlobal(const std::string& name, const StackValue& value, bool isConst = false);
    [[nodiscard]] EzEnvironmentPtr globalEnvironment() const;
    void setGlobalEnvironment(EzEnvironmentPtr env);

    Interpreter& rawInterpreter();
    const Interpreter& rawInterpreter() const;
};
```

#### `initialize()`

Runs top-level script code once.

Use it to:

- define words
- run top-level variable declarations
- execute startup code

Calling `initialize()` more than once is a no-op.

#### `pcallInitialize()`

Protected version of `initialize()`.

Behavior:

- runs top-level startup once
- catches `EzException`
- returns `EzCallResult` with no values on success
- leaves `isInitialized()` false if startup fails

Use this when a host wants to treat top-level chunk startup like a protected Lua-style call.

#### `isInitialized()`

Returns whether top-level execution has already run.

#### `callWord(...)`

Calls a word by name and returns the values the word leaves on the stack above the call base.

Behavior:

- auto-initializes the runtime if needed
- pushes provided args onto the stack
- executes the word
- returns outputs as `std::vector<StackValue>`
- restores stack balance on failure

Throws:

- `EzException`
- other runtime exceptions wrapped as runtime `EzError`

#### `pcallWord(...)`

Protected call version of `callWord(...)`.

Behavior:

- catches `EzException`
- returns `EzCallResult`
- never throws on normal script failure

This is the Lua-like embedding path to prefer in hosts.

#### `registerNativeWord(...)`

Registers a low-level native callback directly on the runtime.

Use this when you want direct stack/interpreter access.

#### `registerHostFunction(...)`

Registers a higher-level host callback directly on the runtime.

This is the preferred host-function registration path for most embeddings.

#### `pushUserData(...)`

Pushes userdata directly onto the underlying interpreter stack.

This is a low-level helper and is mainly useful for stack-oriented host integrations.

#### `setRuntimeLimits(...)`

Replaces the runtime limits after construction and applies them immediately.

This lets the host tighten or relax limits for future `initialize()`, `pcallInitialize()`,
`callWord()`, or `pcallWord()` calls.

#### `hasWord(...)`

Returns whether the runtime knows about a word.

Current behavior:

- returns `false` before initialization
- returns word presence after initialization

#### `hasGlobal(...)`

Checks whether a global exists.

#### `getGlobal(...)`

Reads a global value.

Throws if the global does not exist.

#### `setGlobal(...)`

Creates or updates a global.

Behavior:

- creates the global if it does not exist
- updates the global if it already exists
- throws if the existing global is const
- `isConst` only affects first creation

Globals live inside the runtime's current `EzEnvironment`.

#### `globalEnvironment()`

Returns the runtime's current global environment object.

This is mainly useful when a host wants to inspect or intentionally share one environment across runtimes.

#### `setGlobalEnvironment(...)`

Replaces the runtime's global environment.

Behavior:

- throws if `env` is null
- throws if the runtime has already been initialized
- allows intentional global sharing across runtimes when the same `EzEnvironmentPtr` is reused
- initializes `env->table` if needed

#### `rawInterpreter()`

Advanced escape hatch for direct access to the underlying interpreter.

Use this when you need APIs that are not yet surfaced directly on `EzRuntime`.

Examples:

- advanced stack inspection/manipulation
- template helpers like `getUserData<T>(...)`
- experimental interpreter-specific APIs

Prefer the higher-level runtime API where possible.

## Interpreter Host Hooks

Declared in [Interpreter.h](/Interpreter.h).

These methods are most useful through `runtime.rawInterpreter()`.

### Native Registration

```cpp
void registerNativeWord(const std::string& name,
                        int arity,
                        std::function<ControlSignal(Interpreter&)> fn);

void registerHostFunction(const std::string& name,
                          int arity,
                          EzHostFunction fn);
```

#### `registerNativeWord(...)`

Low-level native registration API.

Use this when you want direct stack/interpreter control.

The native callback receives `Interpreter&` and can manipulate the stack directly.

#### `registerHostFunction(...)`

Higher-level wrapper for host functions.

```cpp
using EzHostFunction =
    std::function<std::vector<StackValue>(const std::vector<StackValue>&)>;
```

Behavior:

- pops `arity` args from the stack
- presents them in left-to-right call order
- calls the host function
- pushes returned values back onto the stack

This is the easiest host-function API to start with.

### Runtime Limits

```cpp
void setRuntimeLimits(size_t instructionBudget,
                      size_t maxCallDepth,
                      std::function<bool()> cancelRequested = {});

void resetRuntimeCounters();
```

`setRuntimeLimits(...)` updates the live interpreter limits.

`resetRuntimeCounters()` resets per-execution counters such as instruction count and call depth.

`EzRuntime` already applies and resets these for normal `initialize()`, `callWord()`, and `pcallWord()` usage.

### Word and Global Access

```cpp
bool hasWord(const std::string& name) const;
std::vector<StackValue> callWord(const std::string& name,
                                 const std::vector<StackValue>& args = {});

bool hasGlobal(const std::string& name) const;
StackValue getGlobal(const std::string& name) const;
void setGlobal(const std::string& name, const StackValue& value, bool isConst = false);
```

These are the underlying interpreter methods used by `EzRuntime`.

Interpreter globals are environment-backed rather than memory-slot-backed.
Most hosts should prefer using the `EzRuntime` wrappers unless they need lower-level control.

### Userdata Helpers

```cpp
void pushUserData(std::shared_ptr<void> handle, std::string typeName);

template <typename T>
std::shared_ptr<T> getUserData(const StackValue& value,
                               const std::string& expectedType) const;
```

#### `pushUserData(...)`

Pushes userdata directly onto the stack.

#### `getUserData<T>(...)`

Type-checks userdata by `typeName` and returns a cast `std::shared_ptr<T>`.

Throws if:

- the value is not userdata
- the type name does not match

## Loader API

Declared in [Loader.h](/D:/projects/tryit/ezlang/Loader.h).

```cpp
struct EzLoadedModule {
    std::string moduleName;
    std::string source;
};

using EzImportResolver =
    std::function<EzLoadedModule(const std::string& fromModule,
                                 const std::string& importPath)>;

std::string expandIncludes(const EzLoadedModule& root, const EzImportResolver& resolver);

EzLoadedModule loadModuleFromFilesystem(const std::filesystem::path& path);

EzLoadedModule resolveModuleFromFilesystem(const std::string& fromModule,
                                           const std::string& importPath);
```

Use this layer when the host wants to control how `# "..."` includes are resolved.

Current CLI behavior:

- loads the root module from disk
- resolves includes from disk
- expands them into one source string before compilation

Embedded hosts can replace that with in-memory or application-specific resolution.

## Example

```cpp
EzEngine engine;

auto program = engine.compile(R\"(
    word square 1
        dup *
    end

    word readScore 0
        @score
    end
)\", "plugin.ez");

EzRuntime runtime = engine.createRuntime(program, EzRuntimeLimits{
    .instructionBudget = 100000,
    .maxCallDepth = 64,
    .cancelRequested = {}
});

runtime.registerHostFunction(
    "host-add",
    2,
    [](const std::vector<StackValue>& args) {
        return std::vector<StackValue>{std::get<int>(args[0]) + std::get<int>(args[1])};
    }
);

runtime.setGlobal("score", 42);
runtime.pcallInitialize();

auto square = runtime.callWord("square", { StackValue{5} });
auto score = runtime.pcallWord("readScore");
```

To intentionally share globals:

```cpp
auto sharedEnv = std::make_shared<EzEnvironment>();

EzRuntime runtimeA = engine.createRuntime(program);
EzRuntime runtimeB = engine.createRuntime(program);

runtimeA.setGlobalEnvironment(sharedEnv);
runtimeB.setGlobalEnvironment(sharedEnv);
```

## Current Gaps

These APIs are usable now, but still evolving.

Areas likely to expand next:

- clearer module-environment API
- environment chaining and module environments on top of `EzEnvironment`
- better userdata convenience wrappers
- stronger VM parity for the embedding path
