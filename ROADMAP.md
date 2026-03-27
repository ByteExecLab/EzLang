# EzLang Embeddable v1 Roadmap

## Summary
- Target a **C++-first embedded scripting engine for untrusted plugins**.
- Optimize for host control, predictable runtime behavior, and clean failure modes rather than language breadth.
- Keep the current interpreter as the semantic source of truth first; bring the VM to parity only after the embedding API and safety model are stable.

## Milestones
### M1. Engine Boundary and Error Model
- Split the reusable engine from the CLI in [`/D:/projects/tryit/ezlang/main.cpp`](D:/projects/tryit/ezlang/main.cpp).
- Replace all `std::exit` paths in parsing/loading/runtime with structured error returns or exceptions caught at the engine boundary.
- Introduce a single host-facing error type with: phase, message, module, line, column, optional snippet, and word call trace.
- Make source loading/import resolution host-provided by interface, with the current filesystem include loader retained only for the CLI.

### M2. Proper Runtime Semantics
- Finish real call frames for words in [`/D:/projects/tryit/ezlang/Interpreter.cpp`](D:/projects/tryit/ezlang/Interpreter.cpp): each word call gets an isolated frame, locals are readable, and `let`/`set` semantics are explicit.
- Define variable lookup precedence for v1: local frame first, then globals, with explicit syntax preserved for globals if you keep `@name`.
- Add a real public “call word” entrypoint so hosts can invoke named script functions directly instead of only running whole files.
- Keep current language surface mostly stable; do not add major new syntax until frame/local semantics are correct.

### M3. Embedding API v1
- Add a host-facing API centered on four types:
    - `EzEngineConfig`: native registrations, import resolver, limits defaults, feature flags.
    - `EzCompiledScript`: compiled source/unit plus metadata.
    - `EzContext`: execution state, globals, stack, limits counters, host userdata.
    - `EzResult<T>` / `EzError`: uniform success/failure transport.
- Suggested C++ API shape:
    - `EzEngine engine(config);`
    - `auto script = engine.compile(source, moduleName);`
    - `auto ctx = engine.createContext();`
    - `ctx.setGlobal("name", value);`
    - `auto result = engine.run(script, ctx);`
    - `auto value = engine.call(ctx, "wordName", args);`
- Suggested native binding shape:
    - `using NativeFn = std::function<EzNativeResult(EzContext&, std::span<const EzValue>)>;`
    - Native functions receive explicit args, return explicit values/errors, and do not manipulate the raw VM stack directly.

### M4. Safety and Host Control
- Add runtime limits required for untrusted scripts:
    - instruction budget
    - max call depth
    - max allocated memory / aggregate size
    - optional wall-clock cancellation hook
- Make stdlib capabilities opt-in by module or binding group; no direct filesystem/process access from core language.
- Move `sys-read-*` and `sys-write*` behind host-provided bindings so the host decides whether IO exists.
- Ensure import resolution is fully host-controlled: no implicit disk access in embedded mode.

### M5. VM and Tooling
- Keep interpreter execution as the reference implementation until tests prove VM parity.
- Expand [`/D:/projects/tryit/ezlang/compiler.h`](D:/projects/tryit/ezlang/compiler.h) and [`/D:/projects/tryit/ezlang/vm.h`](D:/projects/tryit/ezlang/vm.h) only after the host API is stable:
    - compile once, run many
    - same errors and semantics as interpreter
    - same native-binding model
- Add bytecode caching only after parity and stable module identity are in place.

## Public APIs / Types to Add
- `EzValue`
    - wrap current runtime value variants and make it the host-visible value type.
- `EzError`
    - `phase`, `message`, `module`, `line`, `column`, `stackTrace`.
- `EzRuntimeLimits`
    - `instructionBudget`, `maxCallDepth`, `maxHeapObjects`, optional `deadline`/cancel callback.
- `EzImportResolver`
    - callback from module name/path to source text or structured load error.
- `EzNativeRegistry`
    - named host functions registered before compilation/execution.
- `EzContext`
    - per-run mutable state; globals, stack, counters, host userdata.
- `EzEngine`
    - owns compile/run/call operations and default configuration.

## Test Plan
- Parse/load/runtime errors never terminate the host process.
- Word calls create isolated frames and local variables do not leak across calls.
- Global injection from host works for read and write according to v1 rules.
- Import resolver can serve in-memory modules without touching the filesystem.
- Runtime limits stop infinite loops and runaway recursion deterministically.
- Native function failures return structured script-visible errors.
- Same script produces identical observable results in interpreter and VM modes before VM is considered production-ready.

## Assumptions
- v1 embedding target is **C++ only**; a C ABI is deferred until the C++ API settles.
- Priority is **untrusted plugin execution**, so safety and host control outrank syntax expansion.
- Interpreter remains the semantic reference first; VM-first execution is not required for v1.
- Large language features like classes, macros, package management, and JIT stay out of scope until the embedding contract is solid.
