# EzLang

EzLang is a stack-based scripting language with postfix syntax and a host-first runtime model.
It is designed to stay small and explicit like a Forth-style language while becoming easy to embed like Lua.

The language keeps execution simple:
- values live on the stack
- words are the main callable unit
- control flow is explicit
- host applications control natives, imports, runtime limits, and error handling

---

## Current Direction

EzLang is moving toward this embedding model:

1. Compile a source chunk
2. Create a runtime from the compiled chunk
3. Register host functions and inject globals
4. Initialize the runtime once
5. Call words many times with protected or throwing calls

That keeps the language postfix and word-oriented while making the host API feel much closer to Lua.

---

## Core Concepts

- Stack-based execution
- Postfix notation
- Explicit control flow
- User-defined words
- Structured runtime errors
- Host-controlled native functions
- Host-controlled module loading
- Reusable runtime state
- Optional interpreter and VM backends

## API Docs

For the host-facing C++ API reference, see [docs/API.md](/docs/API.md).
For a minimal embedding example, see [examples/embed_basic.cpp](/examples/embed_basic.cpp).

---

## Language Features

### Control Flow
- `if / else / endif`
- `while / do / end`
- `break / continue / return`

### Values
- `int`
- `double`
- `bool`
- `string`
- `nil`
- tables
- userdata

### Variables
- global `var` and `const`
- explicit global load with `@name`
- explicit global store with `value ! name`

### Words
- define words with `word <name> <arity> ... end`
- call words by name
- host applications can call words directly through the runtime API

### Debugging
- `print`
- `trace`
- `--dump-tokens`

---

## Quick Language Tour

```ezlang
10 var counter

word square 1
    dup *
end

word classify 1
    dup 0 < if
        "negative" print
        return
    endif

    dup 0 = if
        "zero" print
        return
    endif

    "positive" print
end

@counter print
5 square print
-2 classify
0 classify
7 classify
```

---

## Tables, Arrays, and Structs

Internally, EzLang now uses one table-like runtime type.
Current surface syntax still exposes array-style and struct-style literals:

Array-style table:

```ezlang
[ 10 20 30 ]
dup array-len print
dup 1 array-get print
1 99 array-set
dup 1 array-get print
drop
```

Struct-style table:

```ezlang
{ "name" "EzLang" "year" 2026 }
dup "name" struct-get print
dup "year" . print
"version" 1 struct-set
dup "version" struct-get print
drop
```

Notes:
- array indexing is currently 0-based
- struct literals currently use alternating key/value items
- `array-get`, `array-set`, `struct-get`, `struct-set`, and `.` are thin table helpers

---

## Embedding EzLang

The intended embedding flow is:

```cpp
EzEngine engine;
auto program = engine.compile(source, "plugin.ez");

auto runtime = engine.createRuntime(program, EzRuntimeLimits{
    .instructionBudget = 100000,
    .maxCallDepth = 256,
    .cancelRequested = {}
});

runtime.rawInterpreter().registerHostFunction(
    "host-add",
    2,
    [](const std::vector<StackValue>& args) {
        return std::vector<StackValue>{
            std::get<int>(args[0]) + std::get<int>(args[1])
        };
    }
);

runtime.setGlobal("score", 42);
runtime.initialize();

auto values = runtime.callWord("update", { StackValue{1}, StackValue{2} });
auto protectedCall = runtime.pcallWord("tick");
```

### Main Embedding Types

- `EzEngine`: compiles source into a reusable program chunk
- `EzRuntime`: owns one interpreter instance and keeps runtime state alive
- `EzRuntimeLimits`: instruction budget, max call depth, and cancellation hook
- `EzCallResult`: protected-call result with `ok`, `values`, and optional error

### Host Integration Features

- register low-level native words with `registerNativeWord(...)`
- register easier host callbacks with `registerHostFunction(...)`
- inject and read globals with `setGlobal(...)` and `getGlobal(...)`
- call script words repeatedly with `callWord(...)`
- catch script failures without exceptions escaping the host by using `pcallWord(...)`

### Userdata

EzLang includes userdata support for passing opaque host-owned values through the runtime.
This is intended for things like engine objects, handles, sockets, files, or game entities.

---

## Errors

EzLang reports structured load, tokenize, parse, and runtime errors.
CLI output includes the message, module, line, column, and source snippet when available.

Example:

```text
[ERROR]: runtime error encountered!
Message      : Unknown word 'foo'
Module       : example.ez
Line         : 3
Column       : 5
    foo bar baz
        ^
```

Embedded hosts can catch `EzException` directly or use protected runtime calls like `pcallWord(...)`.

---

## Modules and Imports

The CLI currently resolves includes from the filesystem.
The engine itself is designed so embedded hosts can provide their own import resolver and serve modules from memory or another source.

This keeps embedding safe and host-controlled instead of forcing direct disk access.

---

## Building

### Requirements
- C++20 compiler
- CMake
- Windows currently has the most testing

### Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

### Run

```bash
./ezlang example.ez
```

Token dump:

```bash
./ezlang --dump-tokens example.ez
```

VM path:

```bash
./ezlang --vm example.ez
```

---

## Design Goals

- small interpreter core
- explicit execution model
- host-controlled embedding surface
- predictable runtime behavior
- reusable runtimes instead of one-shot execution
- structured errors instead of process exits
- Lua-like embeddability without replacing EzLang syntax

---

## Roadmap

### Near Term
- more embedding examples and API docs
- stronger tests for `EzRuntime`, `pcallWord`, and runtime limits
- more table helpers
- better userdata ergonomics
- module environment design

### Mid Term
- lexical locals and better call-frame semantics
- closures / upvalues
- VM parity with interpreter semantics
- bytecode caching for reusable chunks

### Longer Term
- richer standard library
- sandbox and capability controls
- self-hosting experiments
- optional optimizer / JIT work

---

## License

MIT License

---

## Contributing

EzLang is experimental and under active development.
Ideas, feedback, tests, and embedding use-cases are all helpful.
