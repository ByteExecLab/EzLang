#ifndef LEXER_H
#define LEXER_H
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Memory.h"
#include "Stack.h"
#include "Tokenizer.h"

class Interpreter;

/**
 * @enum ControlSignal
 * @brief Indicates non-local control flow during block execution.
 *
 * The `ControlSignal` enum is used internally by the interpreter to manage
 * structured control flow inside looping constructs. Certain tokens such
 * as `continue` and `break` do not behave like normal instructions; instead,
 * they alter the execution flow of the nearest active loop.
 *
 * When a block (such as the body of a `while` loop or an `if` branch)
 * is executed via `executeBlock(...)`, it may return a `ControlSignal`
 * to notify the caller of a special flow redirection.
 *
 * ### Signal Behaviors
 * | Value         | Meaning                                                                                  |
 * |--------------|------------------------------------------------------------------------------------------|
 * | `None`       | Normal execution, no special control flow occurred.                                       |
 * | `Continue`   | Skip the remainder of the current loop iteration and start the next iteration.            |
 * | `Break`      | Immediately terminate the current loop.                                                   |
 *
 * ### Usage Example
 * ```cpp
 * ControlSignal sig = executeBlock(loopBodyTokens);
 * if (sig == ControlSignal::Continue) {
 *     continue; // restart while loop
 * } else if (sig == ControlSignal::Break) {
 *     break;    // exit while loop
 * }
 * ```
 *
 * ### When is a ControlSignal considered an error?
 * A `continue` or `break` outside of any loop context is treated as a
 * runtime error by the interpreter:
 *
 * ```
 * [ERROR]: 'break' used outside of the loop
 * ```
 *
 * ### Related Functions
 * @see Interpreter::executeContinue()
 * @see Interpreter::executeBreak()
 * @see Interpreter::executeBlock()
 */
enum class ControlSignal {
    None, ///< Normal execution, no special control flow.
    Continue, ///< Skip remaining block code and resume next loop iteration.
    Break, ///< Terminate the current loop immediately.
    Return, ///< Return from the current user-defined word.
};

enum class BlockKind { If, While, Word };
struct BlockFrame {
    BlockKind kind;
    Token startToken;
    bool sawElse = false;  // only used for IF
};


struct VariableData {
    uint32_t address;   // Memory Address
    bool isConst;       // true = immutable
};

/**
 * @struct WordDef
 * @brief Represents a user-defined word (function) in the EzLang interpreter.
 *
 * A `WordDef` stores the body of tokens that make up the implementation of a
 * user-defined word, as well as metadata that describes how it interacts
 * with the stack—most importantly, its arity, which specifies how many
 * values must be present on the stack prior to invocation.
 *
 * ### Example
 * ```
 * : square ( n -- n ) dup * ;
 * ```
 * In this example:
 *  - The word name is: `square`
 *  - The body tokens represent: `dup *`
 *  - `arity` is `1`, because the word requires one stack argument.
 *
 * ### Runtime Behavior
 * When a user-defined word is executed:
 *  - The interpreter verifies that the stack contains at least `arity` items.
 *  - If insufficient values are available, a stack underflow error is raised.
 *  - A nested execution context (or `executeBlock(...)`) is used to evaluate
 *    the word’s token body.
 *
 * ### Fields
 * @var body
 * The sequence of tokens that form the word’s implementation. These tokens
 * are executed when the word is invoked.
 *
 * @var arity
 * The required number of arguments that must be available on the stack before
 * the word can run. Defaults to 0 for words that do not require input values.
 *
 * @see Interpreter::executeWordDefinition()
 * @see Interpreter::executeIdentifier()
 */
struct WordDef {
    std::vector<Token> body;
    int arity = 0; // Number of stack arguments required
};

struct NativeWord {
    int arity = 0; // Number of stack arguments required
    std::function<ControlSignal(Interpreter&)> fn;
};

struct Frame {
    // Local variables in this call frame
    std::unordered_map<std::string, StackValue> locals;
};

class Interpreter {
public:
    /**
     * Constructs an Interpreter object with a vector of tokens.
     *
     * This constructor initializes the Interpreter with the sequence of tokens
     * that will be executed. The interpreter uses these tokens to drive its
     * execution process.
     *
     * @param tokens A vector of Token objects representing the program to be
     * interpreted. The constructor takes ownership of this vector.
     * @param stack
     * @param source
     */
    explicit Interpreter(std::vector<Token> tokens, Stack stack, std::string source);

    void registerNativeWord(const std::string& name, int arity, std::function<ControlSignal(Interpreter&)> fn);

    Stack& stack();

    /**
     * Pushes a value onto the stack managed by the Interpreter.
     *
     * This method takes a single value and pushes it onto the stack,
     * ensuring that the stack maintains the sequence of operations
     * as needed during interpretation.
     *
     * @param value The value to be pushed onto the stack. It is provided
     * as a constant reference to avoid unnecessary copying.
     */
    void executePush(const StackValue &value);


    /**
     * Executes the sequence of tokens provided to the Interpreter.
     *
     * This function is the main entry point for the interpreter. It iterates
     * through the vector of tokens, and for each token, performs the
     * corresponding action (e.g., pushing a value onto the stack,
     * performing an arithmetic operation, printing a value).
     *
     * The execution continues until all tokens have been processed.
     *
     */
    void execute();

    /**
     * @brief Validates the structure of control flow blocks in a sequence of tokens.
     *
     * The `validateBlocks` method analyzes a sequence of tokens to ensure that all block
     * structures (e.g., `if`-`else`-`endif`, `while`-`do`-`end`) are properly opened and
     * closed. It verifies the correct nesting of blocks and raises errors if mismatches
     * or unclosed constructs are found.
     *
     * Typically, tokens corresponding to block-related keywords such as `if`, `while`,
     * `do`, `else`, `end`, and custom `word` definitions are processed to maintain a
     * stack representation of open blocks. If an error is detected, such as mismatched
     * keywords or leftover unclosed blocks, the method throws detailed runtime errors.
     *
     * @param tokens A vector of `Token` objects representing the parsed sequence of
     * lexical tokens from the source code. Each token contains information like its type,
     * line number, and column, which are used for error reporting.
     *
     * @throws std::runtime_error if block structures are improperly nested, mismatched,
     * or unclosed. The error message includes the line and column numbers of the problem.
     */
    static void validateBlocks(const std::vector<Token>& tokens);

    /**
     * @var executionMap
     * @brief Maps token types to their associated execution logic in the interpreter.
     *
     * The `executionMap` is a mapping between `TokenType` values and corresponding
     * functions that define the execution behavior for each token type. This is
     * used internally by the interpreter to dynamically dispatch the appropriate
     * execution logic based on the current token being processed during program interpretation.
     *
     * Each entry in the map consists of:
     * - A `TokenType`, which represents a specific type of token in the programming language (e.g., operators, keywords, etc.).
     * - A `std::function<void()>`, which encapsulates the logic to be executed when the associated `TokenType` is encountered.
     *
     * ### Key Roles
     * - Centralizes execution logic for token types into a single accessible structure.
     * - Enables extensibility by allowing new token types and behaviors to be added to the map.
     * - Reduces complexity by abstracting direct conditional branching or duplicated execution logic.
     *
     * ### Internally Used By
     * - The interpreter during runtime to determine and execute the action corresponding to the current token.
     *
     * ### Design Notes
     * - Functions bound to the map should generally perform no side effects outside the intended execution context.
     * - Careful consideration should be given to concurrency and thread safety if the `executionMap` is modified dynamically.
     */
    std::map<TokenType, std::function<void()>> executionMap;
private:
    /**
     * Retrieves the current token and advances the interpreter's position to the next token.
     *
     * This function retrieves the token at the interpreter's current position
     * within the token vector.  It then increments the position, effectively
     * "consuming" the token.
     *
     * @return The token at the interpreter's current position before it was advanced.
     * @throws std::runtime_error if the current position is at or beyond the end of
     * the token vector.
     */
    Token consume();

    /**
     * @brief Executes a sequence of tokens as a nested execution block.
     *
     * This function runs the provided block of tokens using a temporary
     * interpreter state, allowing control structures (such as IF, WHILE,
     * user-defined words, break, and continue) to operate correctly inside
     * nested blocks without altering the caller's token stream position.
     *
     * Execution stops when:
     *  - all tokens in the block are processed, or
     *  - a control signal (ControlSignal::Break or ControlSignal::Continue) is encountered.
     *
     * @param block A vector of tokens representing the code block to execute.
     *
     * @return A ControlSignal value indicating how execution terminated:
     *         - ControlSignal::None     → block finished normally
     *         - ControlSignal::Continue → a `continue` was encountered; caller should restart loop
     *         - ControlSignal::Break    → a `break` was encountered; caller should exit loop
     *
     * Usage Notes:
     * ------------
     * - This method does *not* modify the caller's @c m_tokens or @c m_pos.
     * - Stack operations performed inside the block *do* affect the main stack.
     * - Used internally by control-flow implementations such as IF/ELSE/ENDIF
     *   and WHILE/DO/END.
     */
    ControlSignal executeBlock(const std::vector<Token>& block);

    /**
     * @brief Evaluates the truthiness of a StackValue according to EzLang rules.
     *
     * EzLang uses a simple truthiness model, inspired by Forth and other
     * stack-based languages. This function determines whether a value should be
     * treated as `true` when used in control flow expressions (IF/ELSE/ENDIF,
     * WHILE/DO/END, etc.).
     *
     * Truthiness Rules
     * ----------------
     * The following values are considered **false**:
     *   • Integer: 0
     *   • Float:   0.0
     *   • String:  ""  (empty string)
     *
     * Any other value is considered **true**.
     *
     * @param value A `StackValue` variant containing either int, double, or string.
     * @return `true` if the value is truthy, `false` otherwise.
     *
     * Examples
     * --------
     *     isTruly(0)      → false
     *     isTruly(42)     → true
     *     isTruly(0.0)    → false
     *     isTruly(3.14)   → true
     *     isTruly("")     → false
     *     isTruly("hi")   → true
     *
     * Usage
     * -----
     * This function is primarily used by IF/ELSE and WHILE blocks:
     *
     *     if (isTruly(m_stack.pop())) { ... }
     */
    static bool isTruly(const StackValue& value);

    /**
     * @brief Collects tokens from the current parser position until a matching end token is encountered.
     *
     * This function is used to extract a sequence of tokens forming a block, such as the body of
     * a WHILE or IF statement. It reads tokens starting at the current `m_pos` and stops once
     * a token matching the specified `endType` is found. The ending token is consumed but not
     * included in the returned block.
     *
     * @param endType The token type that marks the end of the block (e.g., TokenType::END,
     *                TokenType::ELSE, or TokenType::ENDIF).
     *
     * @return A vector of `Token` objects representing all tokens found up to (but not including)
     *         the terminating `endType` token.
     *
     * @throws std::runtime_error If the end token is never found before the end of the token list.
     *         The error includes line and column information for easier debugging.
     *
     * Usage
     * -----
     * Typical usage in WHILE/DO/END:
     *
     *     consume(TokenType::WHILE);
     *     auto conditionBlock = collectUntil(TokenType::DO);
     *     consume(TokenType::DO);
     *     auto bodyBlock = collectUntil(TokenType::END);
     *     consume(TokenType::END);
     *
     * Typical usage in IF/ELSE/ENDIF: (simplified)
     *
     *     consume(TokenType::IF);
     *     auto ifBlock = collectUntil(TokenType::ELSE);
     *     consume(TokenType::ELSE);
     *     auto elseBlock = collectUntil(TokenType::ENDIF);
     *     consume(TokenType::ENDIF);
     *
     * Behavior Notes
     * --------------
     * - Nesting is NOT tracked automatically with this function. For nested control structures,
     *   use a more advanced parser such as `collectIfElseEndif()` instead.
     * - The caller is responsible for consuming the `endType` token after this function returns.
     * - If `endType` appears as part of a nested block, this function will stop early unless
     *   nesting-aware logic is used.
     *
     */
    std::vector<Token> collectUntil(TokenType endType);

    /**
     * @brief Retrieves the active call frame of the interpreter.
     *
     * The `currentFrame` method provides access to the most recent call frame in the interpreter's stack.
     * This frame contains the local context for the currently executing function or block of code.
     *
     * ### Behavior
     * - If the call frame stack is empty (i.e., there are no active call frames),
     *   this method will throw a runtime error to signal that no valid frame is available.
     * - Otherwise, it returns a reference to the topmost frame, representing the execution context
     *   at that point in time.
     *
     * ### Error Conditions
     * Throws:
     * - `std::runtime_error`: If the interpreter stack contains no frames.
     *
     * ### Related Concepts
     * The call frame stack is critical to resolving variables and managing execution flow within the interpreter.
     * This method plays a central role in accessing and manipulating the current execution state.
     */
    Frame& currentFrame();

    /**
     * @brief Pushes a new frame onto the call stack.
     *
     * This method adds a new execution frame to the interpreter's stack of active
     * contexts. An execution frame represents the state associated with a specific
     * function or block scope, including local variables and execution metadata.
     *
     * Frames are necessary for maintaining the correct execution and variable scope
     * during nested function calls or block evaluations. Each new invocation of
     * a function or block pushes a fresh frame, isolating its evaluation context
     * from others.
     *
     * ### Context Management
     * - Pushing a frame is typically followed by initialization of local variables,
     *   arguments, or other necessary data for the new scope.
     * - Frames are removed in the reverse order of their insertion, maintaining a
     *   last-in, first-out (LIFO) structure.
     *
     * ### Related Functions
     * @see Interpreter::popFrame()
     * @see Interpreter::currentFrame()
     * @see Interpreter::getVariable()
     */
    void pushFrame();

    /**
     * @brief Removes the top frame from the call stack of the interpreter.
     *
     * The `popFrame` method is used to manage the call stack within the interpreter.
     * It removes the most recently added frame, effectively returning control
     * to the previous execution context. This function is typically invoked
     * when exiting a function or scope within the interpreted program.
     *
     * ### Error Conditions
     * If the call stack (frame stack) is already empty when `popFrame` is called,
     * the method throws a runtime error with the message:
     * `[ERROR]: Frame stack underflow`.
     *
     * ### Context of Use
     * The frame stack is a critical component of the interpreter's state,
     * representing the execution contexts of functions or scopes that are
     * currently active. Proper management of this stack ensures that control
     * flow and variable scoping within the interpreter operate correctly.
     */
    void popFrame();

    /**
     * @brief Collects and separates the token blocks for an IF/ELSE/ENDIF control structure.
     *
     * This function parses the token stream starting at the current interpreter position,
     * extracting the contents of an `if` block and optionally its paired `else` block,
     * until a matching `endif` token is found. The function is *nesting-aware* and can
     * correctly handle nested IF/ELSE/ENDIF constructs by maintaining an internal depth
     * counter. This ensures that inner `endif` tokens do not prematurely terminate the
     * outer IF block.
     *
     * @param ifIndex The position in the token stream where the IF keyword was encountered.
     *        This is used to restore parser state if an error occurs (e.g., unmatched IF).
     *
     * @return A `std::pair` consisting of:
     *         - `first`: A vector of `Token` representing the IF-branch block.
     *         - `second`: A vector of `Token` representing the ELSE-branch block, which
     *           may be empty if no ELSE clause exists.
     *
     * @throws std::runtime_error
     *         - If an `else` is found without a corresponding `if` at the correct depth.
     *         - If more than one `else` appears within the same IF block.
     *         - If the `endif` keyword is missing before the end of input, producing:
     *              `[ERROR]: Unbalanced IF: missing ENDIF before end of input`
     *
     * Parsing Rules & Behavior
     * ------------------------
     * - The `if` keyword that triggered this parsing is considered depth level 1.
     * - Nested IF blocks increase depth; nested ENDIF blocks decrease it.
     * - When depth returns to 0, the function terminates, consuming the `endif`.
     * - The `else` block is only recognized when `depth == 1`. Otherwise, it is
     *   treated as a normal token and copied into the active branch.
     *
     * Example Layout
     * --------------
     *     if <condition>
     *         ... IF branch ...
     *     else
     *         ... ELSE branch ...
     *     endif
     *
     * Usage Example
     * -------------
     *     consume(TokenType::IF);
     *     auto [ifBlock, elseBlock] = collectIfElseEndif(currentIndex);
     *     if (conditionTrue) {
     *         executeBlock(ifBlock);
     *     } else {
     *         executeBlock(elseBlock);
     *     }
     *
     * Limitations
     * -----------
     * - This function assumes the token cursor is positioned *immediately after* an IF token.
     * - The caller must have already popped the condition value from the stack.
     * - No execution or evaluation happens here; this only builds the block structure.
     *
     * See Also
     * --------
     * - `collectUntil(TokenType endType)` – simpler non-nesting block collector
     * - `executeIf()` – runtime execution of IF logic using this function
     *
     */
    std::pair<std::vector<Token>, std::vector<Token>> collectIfElseEndif(size_t ifIndex);


    /**
     * @brief Collects a block of tokens until a matching `end` keyword is encountered.
     *
     * This function is used to extract the body of block-based control structures such as
     * `while ... do ... end`, user-defined `word` definitions, and future structured
     * constructs. It begins collecting from the interpreter's current token position and
     * continues until a corresponding `end` token is found at the same nesting depth.
     *
     * The function is nesting-aware and supports nested blocks. It maintains an internal
     * depth counter so that inner `end` tokens do not prematurely terminate the block.
     *
     * @return A vector of `Token` representing all tokens contained within the block,
     *         excluding the final `end` token that terminates the block.
     *
     * @throws std::runtime_error
     *         - If a matching `end` keyword is not found before the end of token stream.
     *         - If malformed block structure is encountered (e.g., `else` or `endif`
     *           without a parent block, depending on language rules).
     *
     * Block Parsing Rules
     * -------------------
     * - Initial depth is 1 when entering the block parser.
     * - Each nested block keyword (**if**, **while**, **word**, etc.) increments depth.
     * - Each `end` decrements depth.
     * - When depth returns to 0, the block is complete and the function returns.
     * - The terminating `end` is consumed, but not included in the returned token list.
     *
     * Example Usage
     * -------------
     * Used when parsing structures like:
     *
     *     while dup 0 > do
     *         ... body ...
     *     end
     *
     * Example call:
     *
     *     consume(TokenType::WHILE);
     *     auto cond = collectUntil(TokenType::DO);
     *     auto body = collectBlockUntilEnd();
     *     executeWhile(cond, body);
     *
     * Supported Nesting Example
     * -------------------------
     *     while dup 0 > do
     *         if dup 2 = then
     *             ... inner block ...
     *         end
     *     end
     *
     * Limitations
     * -----------
     * - Assumes interpreter position is just *after* the `do`, `word`, or similar
     *   block-start keyword.
     * - Does not validate block semantic correctness—only structural matching.
     *
     * See Also
     * --------
     * - `collectUntil(TokenType endType)`
     * - `collectIfElseEndif(size_t ifIndex)`
     * - `executeWhile()`
     * - `executeIf()`
     */
    std::vector<Token> collectBlockUntilEnd();

    /**
     * Consumes the next token of the specified type from the token stream.
     *
     * This method checks the current token in the stream to verify it matches
     * the expected token type. If the token type does not match or if there are
     * no more tokens to process, an exception is thrown. When the type matches,
     * the token is consumed, and the internal position is incremented.
     *
     * @param tokenType The expected type of the next token in the stream.
     * @param errorMessage The error message to be included in the exception
     * when the token type does not match the expected value.
     * @return The token that was consumed from the stream.
     * @throws std::runtime_error if the token type does not match the expected
     * type, or if the stream has no more tokens.
     */
    Token consume(TokenType tokenType, const std::string& errorMessage);

    /**
     * Peeks at a token ahead of the current position without consuming it.
     *
     * This function allows the interpreter to look at a token in the token stream
     * without advancing its current position.  It's useful for lookahead operations.
     *
     * @param offset The offset from the current position.  An offset of 0 (the default)
     * returns the current token, an offset of 1 returns the next token,
     * and so on.  Must be a non-negative value.
     * @return  A std::optional<Token> representing the token at the specified
     * offset.  If the offset is within the bounds of the token vector,
     * the function returns the token wrapped in an std::optional.
     * If the offset is beyond the end of the token vector, the function
     * returns std::nullptr.
     */
    std::optional<Token> peek(size_t offset = 0);

    /**
     * Executes the "over" operation on the stack.
     *
     * This method duplicates the second-to-top value on the stack and pushes it
     * back onto the stack without altering the original positions of other values.
     * If the stack contains fewer than two elements, an exception is thrown.
     *
     * @throws std::runtime_error If the stack contains fewer than two elements, a stack underflow error is raised.
     */
    void executeOver();

    /**
     * Executes the "nip" operation on the stack.
     *
     * This method removes the second-to-last value from the stack while preserving
     * the last value. It ensures that there are at least two values on the stack
     * before performing the operation. If the stack has fewer than two values,
     * an exception is thrown.
     *
     * @throws std::runtime_error If the stack contains fewer than two values.
     */
    void executeNip();


    /**
     * Executes the 'print' operation, printing the value on the top of the stack.
     *
     * This function retrieves the value from the top of the stack and prints it
     * to the standard output.
     */
    void executePrint();

    /**
     * @brief Prints a detailed snapshot of the current stack contents for debugging.
     *
     * This instruction is used for runtime introspection and debugging of programs.
     * When executed, it outputs a human-readable representation of the stack without
     * modifying its state. The top of the stack is shown last, reflecting push/pop
     * order naturally.
     *
     * The trace output typically appears in the following format:
     *
     *     [TRACE] Stack (size: N):
     *         0: <bottom value>
     *         1: <next value>
     *         ...
     *         N-1: <top value>
     *
     * If the stack is empty, the trace indicates:
     *
     *     [TRACE] Stack is empty
     *
     * The function safely clones the stack before printing, ensuring that no values
     * are popped during inspection.
     *
     * Usage Example (EzLang code):
     * ----------------------------
     *     10 20 30 trace
     *
     * Expected output:
     *     [TRACE] Stack (size: 3):
     *         0: 10
     *         1: 20
     *         2: 30
     *
     * Another example inside a loop:
     *
     *     5 while dup 0 > do
     *         trace
     *         1 -
     *     end
     *
     * Error Behavior:
     * ---------------
     * - Guaranteed not to throw, unless stack internals are corrupted.
     *
     * Design Notes:
     * -------------
     * - Useful for inspecting runtime state while developing programs.
     * - Helps verify correctness of stack manipulation operations.
     * - Plays an important role when diagnosing unexpected interpreter behavior.
     *
     * See Also:
     * ---------
     * - `Stack::getContents()`
     * - `Stack::peek()`
     * - `Stack::pop()` (not used directly here to avoid mutation)
     */
    void executeTrace();

    /**
     * @brief Executes the `continue` control-flow instruction.
     *
     * This instruction is valid only inside a looping context (e.g., `while ... end`).
     * When called, it signals to the interpreter that the current loop iteration should
     * stop immediately and execution should continue from the next iteration of the loop.
     *
     * Behavior:
     * ---------
     * - When encountered inside a loop body, this function does *not* jump by itself.
     *   Instead, it sets and returns a `ControlSignal::Continue` status for the currently
     *   executing `executeBlock()` call.
     * - The loop executor (e.g., `executeWhile()`) detects this signal and restarts the loop
     *   without executing the remainder of the block.
     * - If `continue` is encountered *outside* any loop, this function throws a runtime error:
     *
     *     [ERROR]: 'continue' used outside of a loop
     *
     * Usage Example (EzLang code):
     * ----------------------------
     *     10 while dup 0 > do
     *         dup 2 % 0 = if
     *             "even" print
     *             1 -
     *             continue   ; skip printing raw number
     *         endif
     *         dup print
     *         1 -
     *     end
     *
     * Expected Output:
     *     even
     *     9
     *     even
     *     7
     *     even
     *     5
     *     even
     *     3
     *     even
     *     1
     *
     * Design Notes:
     * -------------
     * - `continue` does not modify the stack by itself; programs should update values
     *   (e.g., decrement counters) manually before issuing `continue`.
     * - Works in conjunction with `executeBlock()` and loop handlers.
     * - Implemented through interpreter-level signaling instead of token jumps,
     *   enabling future optimizations or bytecode compilation.
     *
     * Error Conditions:
     * -----------------
     * This function throws an exception if:
     * - There is no active loop to continue from (not inside `executeWhile()` body).
     *
     * Related Functions:
     * ------------------
     * - `executeBreak()` — terminates the loop entirely
     * - `executeWhile()` — loop execution handler
     * - `executeBlock()` — propagates `ControlSignal` values up the call stack
     */
    void executeContinue();

    /**
     * Executes a string literal token and pushes its value onto the stack.
     *
     * This method processes a string literal token from the token stream,
     * extracts its value, and places it on the interpreter's stack. If the
     * expected token is not a string literal, an error is reported.
     *
     * @throws std::runtime_error if the token is not of type STR_LITERAL.
     */
    void executeStrLiteral();

    /**
     * Executes an integer literal operation in the interpreter.
     *
     * This method processes the next integer literal token from the input sequence and pushes
     * its value onto the runtime stack. It ensures that the token being consumed matches the
     * expected type.
     *
     * An error message is displayed if the expected token type is not found during the parsing process.
     *
     * @throws std::runtime_error If the token type does not match TokenType::INT_LITERAL.
     */
    void executeIntLiteral();

    /**
     * Executes a binary arithmetic operation.
     *
     * This function performs the arithmetic operation specified by the 'tokenType'
     * on the top two values of the stack.
     *
     * @param tokenType The type of binary operation to execute (e.g., ADD, SUB, MUL, DIV, MOD).
     */
    void executeBinary(TokenType tokenType);

    /**
     * Executes a logical operation based on the provided token type.
     *
     * This method performs a logical comparison between the top two values
     * on the stack. Depending on the token type, it evaluates conditions such
     * as equality, inequality, less than, greater than, or their respective
     * inclusive counterparts. The result of the operation is pushed back onto
     * the stack. If the stack contains fewer than two elements, or if the types
     * of the stack values are incompatible for the operation, an exception is thrown.
     *
     * @param tokenType The type of logical operation to execute, represented
     *                  as a `TokenType`. Supported values include equality,
     *                  inequality, less than, greater than, less than or equal to,
     *                  and greater than or equal to.
     */
    void executeLogical(TokenType tokenType);

    /**
     * @brief Handles the execution of `return` statements during script interpretation.
     *
     * This function processes the `return` keyword encountered during interpretation
     * and sets an internal control signal to indicate a return flow. The function does
     * not validate whether the `return` appears in a valid context, leaving that responsibility
     * to the calling code.
     *
     * Upon encountering a `return`, the execution state updates to signal the interpreter that
     * the current function call or execution block should terminate and return control.
     *
     * ### Return Behavior
     * - Sets the internal control signal to `ControlSignal::Return`.
     * - The legality of the `return` statement is evaluated by the calling context.
     *
     * ### Errors
     * If the `return` keyword is not found where expected, the function raises an error:
     * ```
     * [ERROR]: Expected 'return'
     * ```
     *
     * ### Related Enum Values
     * @see ControlSignal::Return
     *
     * ### Related Functions
     * @see Interpreter::executeBlock()
     */
    void executeReturn();

    /**
     * @brief Executes the behavior of a `break` statement within the interpreter.
     *
     * The `executeBreak` function handles the processing of a `break` control statement,
     * ensuring that the current execution flow is redirected to exit the nearest active loop.
     * Upon execution, it assigns a `ControlSignal::Break` to signal a control flow change
     * to the caller of the current block.
     *
     * The function verifies the presence and validity of a `break` keyword in the token stream,
     * and any misuse or invalid context for `break` results in an error.
     *
     * ### Behavior
     * - Immediately halts execution of the current loop containing the `break` statement.
     * - Propagates control to the code following the loop.
     * - Registers a `ControlSignal::Break` to manage this redirection internally.
     *
     * ### Error Conditions
     * - A `break` used outside of any valid loop construct will produce a runtime error:
     *   "[ERROR]: 'break' used outside of the loop".
     *
     * ### Related Components
     * @see ControlSignal::Break
     * @see Interpreter::executeBlock()
     */
    void executeBreak();


    /**
     * Executes a zero-check operation on the value at the top of the stack.
     *
     * This method checks whether the top value of the stack is zero. It pops the top
     * value from the stack, evaluates it, and then pushes a boolean result indicating
     * the outcome of the check (true if the value is zero, false otherwise).
     *
     * An error is thrown if the stack is empty, indicating a stack underflow during
     * the zero-check operation.
     *
     * @throw std::runtime_error If the stack is empty.
     */
    void executeZeroCheck();

    /**
     * Executes an IF-ELSE conditional block in the interpreted code.
     *
     * This method evaluates the condition from the top of the stack and determines
     * whether to execute the tokens inside the IF branch or the ELSE branch. It
     * creates a new interpreter for the respective branch and executes the
     * corresponding set of instructions. If the condition evaluates to true, the
     * IF branch is executed; otherwise, the ELSE branch (if present) is executed.
     *
     * The method ensures tokens are processed correctly, validating the presence
     * of an END token that marks the end of the conditional block. If the block
     * is not properly terminated, or if the stack is empty when evaluating the
     * condition, an exception is thrown.
     *
     * @throws std::runtime_error If the stack is empty while evaluating the
     * condition or if the ENDIF token is missing.
     */
    void executeIf();

    /**
     * Executes a WHILE-DO loop based on the provided tokens.
     *
     * This method processes a structured token sequence representing a WHILE-DO
     * loop. It evaluates the loop condition repeatedly and executes the loop body
     * as long as the condition evaluates to true. The tokens for the condition
     * and body are separated and interpreted individually to allow dynamic execution.
     *
     * - The condition tokens are evaluated on each iteration.
     * - The body tokens are executed if the condition evaluates to true.
     * - The loop terminates when the condition evaluates to false.
     *
     * Proper stack behavior and token organization are assumed during execution.
     * The method ensures that unmatched or missing END tokens result in a runtime
     * error. Stack underflow during condition evaluation also leads to runtime
     * exceptions.
     *
     * @throws std::runtime_error If the END token is missing from the WHILE-DO block.
     * @throws std::runtime_error If the stack underflow's during condition evaluation.
     */
    void executeWhile();

    /**
     * Prints the value held by a std::variant to the standard output stream (std::cout).
     *
     * This function uses std::visit to determine the actual type of the value
     * stored within the variant and then prints that value accordingly. It handles
     * both 'int' and 'std::string' types.
     *
     * @param value A const reference to the std::variant whose value is to be printed.
     */
    static void printVariant(const StackValue& value);

    /**
     * Defines a new variable with an initial value in the interpreter.
     *
     * This method creates a new variable by associating it with a unique memory
     * address. The variable's initial value is taken from the top of the stack.
     * If the variable already exists, an exception is thrown. The new variable's
     * name is obtained from the token stream, and the interpreter ensures the
     * token sequence is valid for a variable definition.
     *
     * @throws std::runtime_error If a variable with the same name is already
     * defined, or if the token sequence is invalid for defining a variable.
     */
    void executeDefineVariable();

    void defineVariable(bool isConst);

    void executeDefineConst();
    void executeDefineVar();

    /**
     * @brief Executes an identifier token, typically representing a user-defined word.
     *
     * This function is invoked when the interpreter encounters a token of type
     * `TokenType::IDENTIFIER`. Identifiers may refer to user-defined words (similar to
     * Forth "words") that have been previously declared, or they may represent variables
     * depending on the language rules.
     *
     * Behavior:
     * ---------
     * 1. The interpreter retrieves the identifier name from the token.
     * 2. It checks whether the identifier corresponds to a registered user-defined word.
     * 3. If found, it executes the associated token block using `executeBlock()`.
     * 4. If the identifier has not been defined, an informative runtime error is thrown.
     *
     * Example (EzLang code):
     * ----------------------
     *     : square dup * end
     *
     *     5 square print   ; outputs: 25
     *
     * In this example, `square` is an identifier that refers to a user-defined word,
     * and calling it invokes the stored token sequence `dup *`.
     *
     * Error Handling:
     * ---------------
     * If the identifier is unknown or not mapped to a user-defined definition,
     * this function throws an error:
     *
     *     [ERROR]: Unknown identifier: <name>
     *
     * Notes:
     * ------
     * - Identifiers are case-sensitive.
     * - This function does not handle variables unless variable lookups are also
     *   implemented through the same mechanism. If variable support is present,
     *   the implementation may check variable storage before failing.
     * - Control signals (`continue` / `break`) originating from inside a user-defined
     *   word are propagated upward via `executeBlock()`.
     *
     * Related Components:
     * -------------------
     * - `defineWord(name, tokens)` — registers a new user-defined word.
     * - `executeBlock(tokens)` — evaluates the token sequence of a word.
     * - `TokenType::IDENTIFIER` — token type associated with identifiers.
     *
     * @throws std::runtime_error if the identifier is not defined as a word or
     *         if its execution results in stack or control flow errors.
     */
    void executeIdentifier();

    /**
     * @brief Executes a word (function) definition introduced by the `:` operator.
     *
     * This instruction enables creation of user-defined words in the EzLang language,
     * similar to Forth-style function definitions. A word definition has the form:
     *
     *     : name   <body tokens>   end
     *
     * Once defined, the word can be invoked like any built-in operation, allowing
     * programs to define reusable operations and abstractions.
     *
     * Definition Rules:
     * -----------------
     * - The definition must begin with `:` (TokenType::DEFINE_WORD).
     * - The next token must be an identifier, which will become the word's name.
     * - All following tokens until a matching `end` are captured as the body of the word.
     * - The definition does **not** execute the body immediately; it is stored for later use.
     * - Attempting to redefine an existing word should produce an error, unless the
     *   implementation explicitly allows overriding.
     *
     * Example (EzLang code):
     * ----------------------
     *     : square dup * end
     *     5 square print   ; outputs: 25
     *
     *     : hypotenuse dup * swap dup * + sqrt end
     *
     * Error Handling:
     * ---------------
     * This function throws a runtime error if:
     * - The word name is missing or not a valid identifier.
     * - The definition is not terminated with `end` before end of token stream.
     * - A name is already defined and redefinition is not allowed (implementation-dependent).
     *
     * Stored Word Behavior:
     * ---------------------
     * Defined words are stored in a dictionary-like structure, e.g.:
     *
     *     std::unordered_map<std::string, std::vector<Token>> m_userWords;
     *
     * Calling a word later executes its stored token list via `executeBlock()`.
     *
     * Related Components:
     * -------------------
     * - `void executeIdentifier()` — invoking a user-defined word.
     * - `std::vector<Token> collectBlockUntilEnd()` — used internally to capture the word body.
     * - `executeBlock(tokens)` — executes the tokens that represent the body.
     *
     * Grammar Fragment:
     * -----------------
     *     word-definition ::= ":" IDENTIFIER token-sequence "end"
     *
     * @throws std::runtime_error if the syntax is malformed or the block is unterminated.
     */
    void executeWordDefinition();

    /**
     * Executes the operation to load a variable from memory onto the stack.
     *
     * This method retrieves the value of a named variable from the memory, using
     * its address, and pushes the value onto the stack for further use. The variable
     * to be loaded is specified in the current sequence of tokens being processed.
     * If the variable is not defined, an exception is thrown.
     *
     * Proper token consumption is ensured during the execution process, expecting
     * a valid identifier followed by the load variable operation token.
     *
     * @throws std::runtime_error If the specified variable is not defined.
     */
    void executeLoadVariable();

    /**
     * Executes the operation to store a value into a pre-defined variable in memory.
     *
     * This method retrieves a variable name from the current token in the token sequence
     * and verifies its existence in the variable map. The corresponding value from the
     * stack is stored at the memory address associated with the variable. The stack
     * must not be empty before execution, and appropriate tokens must follow
     * the language's expected syntax.
     *
     * @throws std::runtime_error If the stack is empty, the variable is not defined,
     * or the required tokens do not follow the expected sequence.
     */
    void executeStoreVariable();

    /**
     * @brief Executes the `let` statement in the interpreter, creating or updating a local variable.
     *
     * The `executeLet` method processes the `let` keyword in the language, which declares a new variable
     * or updates the value of an existing one in the current frame's local variable scope. This method
     * consumes the `let` token, extracts the variable's name, and assigns it the top value from the
     * operand stack.
     *
     * ### Behavior and Constraints
     * - **Token Expectations**: The method expects a valid `TokenType::LET` followed by an
     *   `TokenType::IDENTIFIER`, representing the variable name.
     * - **Stack Dependency**: A value must already exist on the stack prior to calling `executeLet`.
     *   If the stack is empty, the method throws a runtime error to indicate stack underflow.
     * - **Variable Storage**: The popped value is stored in the `locals` map of the current stack frame,
     *   with the variable name as the key.
     *
     * ### Error Handling
     * Some runtime errors may occur under the following conditions:
     * - If the `let` keyword or a valid identifier is missing, a syntax error is thrown.
     * - If the stack is empty when attempting to pop a value for assignment, this results in a
     *   "[INTERPRETER][ERROR]: Stack underflow" error.
     *
     * ### Related Functions
     * @see Interpreter::consume()
     * @see Interpreter::currentFrame()
     */
    void executeLet();

    /**
     * @brief Executes a `set` operation in the interpreter's execution environment.
     *
     * The `executeSet` function is responsible for handling assignment operations.
     * It evaluates the left-hand side (e.g., a variable or property) and assigns it
     * a value determined by evaluating the right-hand side expression.
     *
     * This function operates within the context of the interpreter and modifies
     * state accordingly, such as updating variable bindings in the current scope
     * or producing runtime errors for invalid operations.
     *
     * ### Behavior
     * - The function resolves the target of the assignment by evaluating the left-hand side.
     * - It evaluates the right-hand side expression to compute the value.
     * - The computed value is assigned to the resolved target.
     *
     * ### Error Handling
     * Executes validations to ensure correct assignment semantics:
     * - Throws a runtime error if attempting to assign to an invalid target (e.g., an undeclared variable or read-only property).
     * - Produces a type error if assignment violates type constraints (if applicable).
     *
     * ### Related Functions
     * @see Interpreter::evaluateExpression()
     * @see Interpreter::resolveIdentifier()
     */
    void executeSet();

    /**
     * @brief Executes the logical AND operation in the interpreter.
     *
     * The `executeAnd` function processes the "AND" operation by consuming the
     * relevant token, ensuring a sufficient stack size, and evaluating the logical AND
     * between the two topmost values on the stack. The resulting boolean value is then
     * pushed back onto the stack as an integer (1 for true, 0 for false).
     *
     * ### Behavior
     * - The function first validates that the current token is of type `AND` and consumes it.
     * - It ensures at least two values are present on the stack before proceeding.
     * - The function evaluates the logical AND operation by checking the truthiness of the
     *   two popped stack values using the `isTruly()` method.
     * - The result of the AND operation is converted to an integer and pushed back onto the stack.
     *
     * ### Error Handling
     * - If fewer than two values exist on the stack, the function raises an error
     *   indicating insufficient stack size.
     * - If the token type does not match `AND`, an error is thrown stating the expected token.
     *
     * ### Related Functions
     * @see Interpreter::isTruly()
     * @see Interpreter::consume()
     * @see Stack::pop()
     * @see Stack::push()
     */
    void executeAnd();

    /**
     * @brief Executes a logical "or" operation for the top two values on the stack.
     *
     * This method performs a short-circuiting logical "or" operation between the two
     * most recent values on the interpreter's stack. It evaluates the truthiness of
     * both operands and pushes the resulting boolean value (as an integer: 0 for `false`
     * and 1 for `true`) back onto the stack.
     *
     * ### Operation Details
     * - Consumes and validates an `OR` token.
     * - Ensures the stack contains at least two values prior to performing the operation.
     * - Pops the top two values from the stack to evaluate the truthiness of the first
     *   operand (`a`) and the second operand (`b`).
     * - Applies short-circuit logic: if the truthiness of `a` is `true`, the second operand
     *   (`b`) is not evaluated.
     * - Pushes the computed result of the logical "or" operation back onto the stack.
     *
     * ### Stack Behavior
     * Before execution:
     * | Stack                 |
     * |-----------------------|
     * | ...                   |
     * | Operand A             |
     * | Operand B             |
     *
     * After execution:
     * | Stack                 |
     * |-----------------------|
     * | ...                   |
     * | Result (0 or 1)       |
     *
     * ### Error Handling
     * Throws an error if fewer than two elements are available on the stack before the
     * operation or if the `OR` token is missing.
     *
     * ### Related Information
     * @see Interpreter::executeAnd()
     * @see Interpreter::isTruly()
     */
    void executeOr();

    /**
     * @brief Executes a logical NOT operation on the top value of the stack.
     *
     * This method evaluates the truthiness of the top-most value on the execution stack
     * and pushes the negated boolean result back onto the stack. The truthiness of a value
     * is determined using the internal `isTruly` function.
     *
     * ### Operational Details
     * - A single value is consumed from the stack.
     * - The operation validates that there is at least one value on the stack before execution.
     * - The result of the NOT operation is pushed as an integer, where `1` represents "true"
     *   and `0` represents "false".
     *
     * ### Preconditions
     * - The stack must contain at least one value prior to invocation.
     * - The token `NOT` is expected, failing which the method throws an error with the
     *   appropriate message.
     *
     * ### Error Handling
     * - If the stack does not contain enough values for this operation, an error is raised.
     * - An error is also triggered if the expected `NOT` token is not present when this
     *   operation is invoked.
     *
     * ### Related Functions
     * @see Interpreter::isTruly()
     */
    void executeNot();

    /**
     * @brief Begins the execution of an array construction.
     *
     * The `executeArrayStart` method initializes the processing of an array by
     * validating the opening bracket (`[` token) and marking the current state
     * of the interpreter's stack to track elements of the new array.
     *
     * ### Behavior
     * - Expects and consumes the `[` token, which signifies the start of an array.
     * - Records the current stack size in the `m_arrayMarks` list, which is used to
     *   determine the boundary of the array's element collection process.
     *
     * ### Errors
     * Throws an exception if the next token is not the expected `[` token, typically
     * indicating malformed input or a syntax error in the source being interpreted.
     *
     * ### Related Concepts
     * This function works in conjunction with other array processing methods, such as:
     * - Array element parsing and validation.
     * - Proper termination of the array using the closing bracket `]`.
     *
     * @see Interpreter::executeArrayEnd()
     */
    void executeArrayStart();

    /**
     * @brief Finalizes the construction of an array during interpretation.
     *
     * The `executeArrayEnd` method is responsible for handling the closing
     * bracket (`]`) of an array definition in the interpreted code. It ensures
     * that the array structure is correctly constructed and pushed onto the
     * interpreter's evaluation stack, while validating the internal state to detect
     * mismatched or redundant array delimiters.
     *
     * ### Behavior
     * 1. Consumes the `ARRAY_END` token (`]`) from the input stream and ensures
     *    proper syntax by verifying that it matches a previously encountered
     *    array start (`[`).
     * 2. Checks for a corresponding array start mark in the stack. Throws a runtime
     *    error if no matching `[` was found (`[ERROR]: Unmatched ']' with no '['`).
     * 3. Retrieves all stack values added to the array between `[` and `]`, respecting
     *    the left-to-right order of array elements.
     * 4. Constructs an `ArrayValue` object with the gathered elements in the correct
     *    order.
     * 5. Pushes the newly created array onto the evaluation stack.
     *
     * ### Errors
     * - Throws if the `ARRAY_END` token does not match a preceding `ARRAY_START`.
     * - Throws an internal runtime error if the state of the interpreter's stack
     *   is invalid, such as when the stack contains fewer elements than expected.
     *
     * ### Related Functions
     * @see Interpreter::executeArrayStart()
     * @see Interpreter::consume()
     * @see ArrayValue
     */
    void executeArrayEnd();

    /**
     * @brief Begins the interpretation of a structured block, such as a compound
     *        statement or a control flow structure.
     *
     * This method prepares the interpreter for the start of a new structured block by
     * consuming the appropriate starting token (e.g., a `{` token) from the input
     * stream. Additionally, it marks the current state of the evaluation stack,
     * allowing the interpreter to manage and isolate scope-specific variables or
     * operations within the block.
     *
     * ### Key Operations
     * - Validates the presence of the expected starting token for the structured block.
     * - Records the current state of the stack in `m_structMarks` for later use, such as
     *   scope cleanup or evaluation boundary management.
     *
     * ### Error Handling
     * If the starting token is missing or invalid, this method raises a runtime error
     * signaling an unexpected token or malformed input.
     *
     * ### Related Functions
     * @see Interpreter::executeStructEnd()
     * @see Interpreter::consume()
     */
    void executeStructStart();

    /**
     * @brief Finalizes the construction of a structured literal, such as a map or struct.
     *
     * The `executeStructEnd` method is invoked when the interpreter encounters the closing
     * brace (`}`) of a struct literal. It performs validation checks to ensure proper
     * syntax and semantics of the struct and processes the key-value pairs from the
     * execution stack into a structured object.
     *
     * ### Key Operations
     * - Ensures that the closing brace matches a previously encountered opening brace (`{`).
     * - Verifies that the number of elements on the stack corresponds to valid key-value
     *   pairs (even number of items).
     * - Checks that all keys are of string type, as required by struct definitions.
     * - Constructs a `StructValue` object from the parsed key-value pairs.
     * - Pushes the newly constructed struct object back onto the execution stack.
     *
     * ### Error Handling
     * The method can throw runtime errors under the following conditions:
     * - When there is a closing brace (`}`) without a corresponding opening brace (`{`).
     * - If the stack size is smaller than expected based on the struct start marker.
     * - If the number of items between the struct start and end is not even.
     * - If any key in the struct is not of string type.
     *
     * ### Related Behavior
     * Struct literals are defined and delimited using `{` and `}` in the interpreted language.
     * Pairs of values pushed onto the stack between these delimiters are treated as
     * key-value pairs, forming the data of the struct.
     *
     * ### Example Errors
     * - Unmatched closing brace:
     *   ```
     *   [ERROR]: Unmatched '}' with no '{'
     *   ```
     * - Mismatched stack size:
     *   ```
     *   [ERROR]: Internal: stack smaller than struct start mark
     *   ```
     * - Odd number of items:
     *   ```
     *   [ERROR]: Struct literal expects key/value pairs (even number of stack items)
     *   ```
     * - Non-string key:
     *   ```
     *   [ERROR]: Struct keys must be strings
     *   ```
     *
     * ### Stack and Mark Management
     * The `m_structMarks` list tracks stack positions where struct parsing begins.
     * After successful parsing, the mark is removed, and the constructed object is
     * pushed onto the stack.
     */
    void executeStructEnd();

    /**
     * @brief Computes the length of an array on the stack and pushes the result.
     *
     * The `executeArrayLen` function is responsible for calculating the size of an array
     * stored as the topmost value on the interpreter's stack. This function first validates
     * that the appropriate token type (`ARRAY_LEN`) is present and then proceeds to check
     * the stack state. If the stack is empty or the top value is not an array, a runtime
     * error is generated. Otherwise, this function determines the length of the array and
     * pushes the resulting integer back onto the stack.
     *
     * ### Execution Flow
     * 1. Verifies that the `ARRAY_LEN` token is present.
     * 2. Ensures there is a value on the stack to operate on.
     * 3. Checks if the top value of the stack is a valid array type.
     * 4. Computes the number of elements in the array.
     * 5. Pushes the computed length back onto the stack.
     *
     * ### Error Handling
     * - Throws a runtime error if the stack is empty when attempting to retrieve the value.
     * - Throws a runtime error if the top value of the stack is not an array.
     *
     * ### Related Functions
     * @see Interpreter::consume()
     * @see Interpreter::m_stack
     */
    void executeArrayLen();

    /**
     * @brief Executes the `array-get` operation to retrieve an element from an array by index.
     *
     * This method is responsible for handling the `array-get` operation in the interpreter.
     * It pops two elements from the stack: the array and the index, verifies their validity,
     * and pushes the corresponding array element back to the stack.
     *
     * The method performs the following validations and operations:
     * - Ensures that there are at least two elements on the stack before execution.
     * - Verifies that the second-to-top stack value is a valid array.
     * - Extracts the index from the top stack value and ensures it is within the bounds of the array.
     * - If all conditions are met, pushes the array element located at the specified index back onto the stack.
     *
     * ### Error Conditions
     * If any of the following conditions occur, the method will throw a runtime exception:
     * - Insufficient stack size (less than two elements).
     * - The stack does not contain a valid array at the expected position.
     * - The specified index is not an integer.
     * - The index is out of bounds for the array.
     *
     * ### Stack Behavior
     * Before Execution:
     * - The stack must contain an array and an index as the top two elements.
     *
     * After Successful Execution:
     * - The original array and index are removed from the stack.
     * - The result (array element) is pushed onto the stack.
     *
     * ### Related Functions
     * @see Interpreter::executeArraySet()
     * @see Interpreter::executeArrayPush()
     * @see Interpreter::executeArrayPop()
     */
    void executeArrayGet();

    /**
     * @brief Handles the `array-set` operation in the interpreter.
     *
     * The `executeArraySet` method sets the value of an element in an array
     * at a specified index. This operation requires three values on the stack:
     * the array, the target index, and the value to set. The updated array
     * is then pushed back onto the stack.
     *
     * ### Stack Requirements
     * Before execution:
     * - The top of the stack must be the value to assign.
     * - The second element must represent the index.
     * - The third element must be the array to modify.
     *
     * After execution:
     * - The stack will contain the updated array with the value modified
     *   at the specified index.
     *
     * ### Errors
     * The following runtime errors may occur during execution:
     * - If the stack contains fewer than three elements, an error is thrown:
     *   `[ERROR]: array-set requires array, index, value`.
     * - If the provided array is not of the expected type, an error is thrown:
     *   `[ERROR]: array-set expects array under index/value`.
     * - If the provided index is out of bounds, an error is thrown:
     *   `[ERROR]: array-set index out of range`.
     *
     * ### Implementation Notes
     * - The index must be a valid integer. If it is not, an error is raised.
     * - This method uses `std::variant` to ensure type safety while manipulating
     *   stack values.
     * - The input array is copied and modified before pushing the updated version
     *   back to the stack.
     *
     * ### Related Functions
     * @see Interpreter::executeArrayGet()
     */
    void executeArraySet();

    /**
     * @brief Executes the `struct-get` operation to retrieve a value from a structure by key.
     *
     * This method performs a runtime operation to extract a field's value from a structure
     * using a string key. It is designed to handle runtime stack manipulation within
     * the interpreter.
     *
     * ### Expected Input
     * The operation expects exactly two values on top of the interpreter's stack:
     *  1. A key (`std::string`): The field name to retrieve.
     *  2. A structure (`StructValue`): The structure containing the desired field.
     *
     * ### Runtime Behavior
     *  - If the key is not of type `std::string`, an exception is thrown with the
     *    message: "[ERROR]: struct-get expects string key".
     *  - If the second value is not a `StructValue`, an exception is thrown with the
     *    message: "[ERROR]: struct-get expects struct under key".
     *  - If the key exists in the structure, the corresponding value is pushed onto the stack.
     *  - If the key does not exist, a `std::monostate` is pushed onto the stack to indicate
     *    absence of the field.
     *
     * ### Error Handling
     *  - If there are fewer than two values on the stack when `struct-get` is invoked,
     *    an exception is thrown with the message: "[ERROR]: struct-get requires struct and key".
     *
     * ### Stack State
     * Before execution:
     *  - Top-1: `std::string` key
     *  - Top-2: `StructValue` object
     *
     * After execution:
     *  - Top: The retrieved value or `std::monostate` if the key does not exist.
     *
     * ### Usage Notes
     * This method is commonly used to access fields dynamically in structured execution contexts
     * such as when interpreting user-defined structures during runtime.
     */
    void executeStructGet();

    /**
     * @brief Updates or adds a key-value pair within a struct object on the interpreter stack.
     *
     * The `executeStructSet` method is responsible for modifying a struct object by setting
     * a new key-value pair or updating an existing one. This operation assumes that the stack
     * contains at least three elements: the struct object, the key (as a string), and the value
     * to be associated with the key. If the top three stack elements do not meet these conditions,
     * the operation throws a runtime error.
     *
     * ### Preconditions
     * - The stack must contain at least three elements.
     * - The top stack element must be the value to set.
     * - The second element from the top must be the key, which should be a string.
     * - The third element from the top must be the struct, which must be a valid struct object.
     *
     * ### Postconditions
     * - The modified struct is pushed back onto the stack in place of the original struct.
     * - The original struct and its associated key-value pair are updated or created without
     *   affecting other fields in the struct.
     *
     * ### Error cases
     * The following errors may occur if the input does not meet preconditions:
     * - If the stack contains fewer than three elements, an error is thrown:
     *   `[ERROR]: struct-set requires struct, key, value`.
     * - If the key is not a string type, an error is thrown:
     *   `[ERROR]: struct-set expects string key`.
     * - If the struct is not of the required type, an error is thrown:
     *   `[ERROR]: struct-set expects struct under key/value`.
     *
     * ### Related Functions
     * @see Interpreter::executeStructGet()
     * @see Interpreter::executeStructCreate()
     */
    void executeStructSet();

    /**
     * @brief Executes a struct field access operation.
     *
     * This method handles the `.` (dot) operator used for accessing fields within a struct.
     * It consumes the `.` token and retrieves the necessary operands (struct and field key)
     * from the stack to perform the access operation.
     *
     * ### Stack Behavior
     * - Before calling this function, the top of the stack must contain:
     *   - A string representing the key (field name).
     *   - A struct object containing the fields to access.
     * - After execution:
     *   - The value of the requested field is pushed onto the stack if the key exists.
     *   - If the key is not found, a default `std::monostate` value is pushed onto the stack.
     *
     * ### Error Conditions
     * This method throws a runtime error in the following cases:
     * - If the stack contains fewer than two elements when `.` is encountered.
     * - If the top of the stack is not a string (invalid key).
     * - If the second element from the top is not a struct object.
     *
     * ### Example Error Messages
     * - `[ERROR]: '.' requires struct and key on stack`
     * - `[ERROR]: '.' expects string key on top`
     * - `[ERROR]: '.' expects struct under key`
     *
     * ### Operation Details
     * - The method retrieves and validates the operands from the stack.
     * - It searches for the specified key in the struct's field map.
     * - If the key exists, its associated value is pushed onto the stack.
     * - If the key does not exist, the function pushes a `std::monostate` to indicate a missing field.
     *
     * ### Related Components
     * @see StructValue
     * @see TokenType::STRUCT_ACCESS
     */
    void executeStructAccess();


    /**
     * Checks if the given StackValue is of a specific type.
     *
     * This method determines whether the provided StackValue object
     * matches the type specified by the template parameter T.
     *
     * @param value The StackValue object to check the type of.
     * @return True if the value matches the specified type, otherwise false.
     */
    template<class T>
    static bool isOfType(StackValue &value);


    /**
     * Retrieves the integer value from a variant, or throws an exception if the variant
     * does not hold an integer.
     *
     * @param value A const reference to the std::variant from which to retrieve the
     * integer value.
     * @return The integer value held within the variant.
     * @throws std::runtime_error if the variant does not hold an integer value.
     */
    static int GetIntOrThrow(const StackValue& value);

    /**
     * Retrieves the string value from a variant, or throws an exception if the variant
     * does not hold a string.
     *
     * @param value A const reference to the std::variant from which to retrieve the
     * string value.
     * @return The string value held within the variant.
     * @throws std::runtime_error if the variant does not hold a string value.
     */
    static std::string GetStringOrThrow(const StackValue& value);

    /**
     * Check if value is numeric value
     *
     * @param v Value to check
     * @return The boolean
     */
    static bool isNumber(const StackValue& v);

    /**
     * @brief Converts a StackValue into a double precision floating-point number.
     *
     * This utility function is used internally by arithmetic, comparison, and other
     * numeric operations to normalize numeric types into a common `double` value.
     *
     * Supported conversions:
     * ---------------------
     * | StackValue type       | Conversion behavior                  |
     * |-----------------------|--------------------------------------|
     * | `int`                 | converted via `static_cast<double>`  |
     * | `double`              | returned directly                    |
     * | `std::string`         | runtime error unless string is a valid numeric literal |
     *
     * Usage Example (internal):
     * -------------------------
     *     StackValue a = 42;
     *     StackValue b = 3.5;
     *
     *     double x = toDouble(a);   // 42.0
     *     double y = toDouble(b);   // 3.5
     *
     * Error Handling:
     * ---------------
     * If the value contains a string that cannot be parsed as a number,
     * the function throws a runtime error:
     *
     *     [ERROR]: Cannot convert string to number: "<value>"
     *
     * Design Notes:
     * -------------
     * - Centralizes numeric promotion logic so arithmetic operations
     *   do not need to type-check manually.
     * - Enables mixed numeric operations (e.g., `3 2.5 +`).
     * - Useful for enhancing the interpreter toward IEEE-754 floating-point behavior.
     *
     * Related Functions:
     * ------------------
     * - `static bool isTruly(const StackValue& value)` — truthiness rules
     * - `static int toInt(const StackValue& v)` — if implemented, for integer normalization
     * - `executeBinary(TokenType)` — uses this helper for arithmetic resolution
     *
     * @param v The StackValue to convert.
     * @return The numeric value as `double`.
     *
     * @throws std::runtime_error If the value is not numeric and cannot be converted.
     */
    static double toDouble(const StackValue& v);

    static bool toBool(const StackValue& v);

    /**
     * @brief Determines whether two StackValue instances both contain integer values.
     *
     * This helper function checks if *both* operands are of type `int`. It is used
     * primarily in arithmetic and comparison operations to choose the most efficient
     * evaluation path and avoid unnecessary floating-point promotion.
     *
     * Typical Use Cases:
     * ------------------
     * - When performing integer-only operations:
     *      - Faster execution
     *      - More predictable results (no floating point rounding issues)
     * - When deciding whether modulo `%` can be applied (only defined for integers)
     * - When optimizing comparisons and math operators
     *
     * Example:
     * --------
     *     StackValue x = 10;
     *     StackValue y = 3;
     *     StackValue z = 2.5;
     *
     *     bothInt(x, y);  // true
     *     bothInt(x, z);  // false
     *     bothInt(z, y);  // false
     *
     * Why This Function Exists:
     * -------------------------
     * - Keeps type checking logic centralized instead of repeating
     *   `std::holds_alternative<int>(...)` everywhere.
     * - Makes arithmetic code cleaner and more readable.
     * - Supports future extension if integers gain a distinct internal type.
     *
     * Related Utility Functions:
     * --------------------------
     * - `static double toDouble(const StackValue& v)` — numeric promotion
     * - `static bool isTruly(const StackValue& value)` — truthiness rules
     * - (optional future) `static bool bothNumeric(const StackValue& a, const StackValue& b)`
     *
     * @param a First value to check.
     * @param b Second value to check.
     * @return true if and only if both values hold integers, otherwise false.
     */
    static bool bothInt(const StackValue& a, const StackValue& b);

    /**
     * @brief Executes the token at the current instruction pointer and advances execution.
     *
     * This function reads the token located at `m_tokens[m_pos]`, performs the
     * corresponding operation, and then increments the instruction pointer
     * (`m_pos`) unless control flow rules dictate otherwise.
     *
     * The interpreter delegates execution to specialized handler functions
     * based on the token type (e.g., arithmetic, stack manipulation, conditionals,
     * loop operations, printing, word execution, etc.).
     *
     * Control Signal Behavior
     * -----------------------
     * The function returns a `ControlSignal` value, which informs callers
     * (such as `executeBlock()` or `execute()` loops) that a control-flow
     * altering action has been triggered:
     *
     * | Signal                         | Meaning |
     * |-------------------------------|---------|
     * | `ControlSignal::None`         | Normal execution, no special control required |
     * | `ControlSignal::Continue`     | Loop body should stop and restart next iteration |
     * | `ControlSignal::Break`        | Loop should terminate immediately |
     *
     * Usage Context
     * -------------
     * - This function is the core execution step for **all** token handling.
     * - It should never be called recursively.
     * - Used by:
     *    - `execute()` for full program execution
     *    - `executeBlock()` for nested execution environments (IF/WHILE/user-defined words)
     *
     * Error Handling
     * --------------
     * Runtime errors thrown here are caught by `execute()`, which reports the
     * source line, column, token type, and a code snippet if available.
     *
     * Examples (Pseudo-Execution Flow)
     * --------------------------------
     * ```
     * while (m_pos < m_tokens.size()) {
     *     ControlSignal sig = executeSingleToken();
     *     if (sig == ControlSignal::Break)    break;
     *     if (sig == ControlSignal::Continue) continue;
     * }
     * ```
     *
     * Examples of Token Delegation:
     * ------------------------------
     * | Token Type         | Calls Handler Function |
     * |-------------------|------------------------|
     * | `ADD` (+)         | `executeAdd()`         |
     * | `PRINT`           | `executePrint()`       |
     * | `IF`              | `executeIf()`          |
     * | `WHILE`           | `executeWhile()`       |
     * | `IDENTIFIER`      | `executeIdentifier()`  |
     * | `WORD_DEF` (: ...) | `executeWordDefinition()` |
     * | `CONTINUE`        | returns `ControlSignal::Continue` |
     * | `BREAK`           | returns `ControlSignal::Break` |
     *
     * Returns:
     * --------
     * @return ControlSignal indicating whether execution continues normally,
     *         or whether a loop requests `break` or `continue`.
     */
    ControlSignal executeSingleToken();

    /**
     * @brief Displays a highlighted snippet of source code around a runtime error location.
     *
     * This helper function is used by the interpreter's exception reporting system
     * to show the problematic line of source code and visually mark the exact column
     * where the execution error occurred. It prints:
     *
     *  1. The full line of code that contains the error
     *  2. A caret (`^`) positioned under the failing column index
     *
     * This makes debugging significantly easier for the user by pointing directly
     * to the source location that triggered the failure.
     *
     * ### Example Output
     * ```
     *   10  5 3 / print
     *              ^
     * ```
     *
     * ### Parameters
     * @param line   The one-based line number where the error occurred.
     *               It must correspond to the `Token.line` value of the
     *               token that caused the runtime failure.
     *
     * @param column The one-based column index of the character associated
     *               with the failing token. It must correspond to the
     *               `Token.column` value.
     *
     * ### Behavior Notes
     * - If the requested line does not exist in the stored source text,
     *   the function prints: `<<unable to display source context>>`
     * - Tabs are not expanded; caret alignment is based on raw characters.
     * - This function only prints context; it does not throw or terminate execution.
     *
     * ### Usage Context
     * Typically invoked inside exception handling, e.g.:
     * ```
     * catch (const std::runtime_error& e) {
     *     std::cerr << e.what() << "\n";
     *     printRuntimeErrorContext(tok.line, tok.column);
     * }
     * ```
     *
     * ### Output Stream
     * The snippet is printed to `std::cerr` so it does not interfere with
     * normal program output on `stdout`.
     */
    void printRuntimeErrorContext(size_t line , size_t column) const;

    /**
     * @brief Ensures that the interpreter's stack contains the minimum required number of values.
     *
     * This method verifies the size of the stack to guarantee that sufficient values
     * are available before certain operations are performed. If the stack does not
     * contain enough elements, it throws a `std::runtime_error` to reflect a stack
     * underflow condition.
     *
     * @param needed The minimum number of values that must be present on the stack.
     * @param opName The name of the operation that requires the specified stack size.
     *
     * @throws std::runtime_error If the stack contains fewer elements than required.
     */
    void ensureStackSize(size_t needed, const std::string& opName) const;

    /**
     * Retrieves the current token being processed by the interpreter.
     *
     * This method returns the token at the current position in the sequence of
     * tokens that the interpreter is processing.
     *
     * @return The token at the current position in the token sequence.
     */
    [[nodiscard]]
    Token getCurrentToken() const;

private:

    /**
     * @brief Holds the full original source code being interpreted.
     *
     * This string contains the entire program text that the tokenizer and
     * interpreter operate on. It is preserved in its original form to enable:
     *
     *  - Accurate error reporting with line and column context
     *  - Runtime debugging and trace output
     *  - Future features such as source mapping, macro expansion, or
     *    interactive evaluation (REPL)
     *
     * The string is indexed by `m_pos` and accessed via `peek()` and `consume()`
     * during tokenization. Newlines increment the internal line/column counters
     * used for user-facing error diagnostics.
     *
     * @note This value is never modified after construction. It is a canonical
     *       view of the program text and should remain immutable.
     *
     * @see printErrorContext()
     * @see Token::line
     * @see Token::column
     */
    std::string m_source;

    /**
     * @var m_controlSignal
     * @brief Represents the current control signal status within the interpreter.
     *
     * The `m_controlSignal` member variable is used internally by the interpreter
     * to track structured control flow signals during the execution of looping
     * constructs or conditional branches. Its primary purpose is to store
     * the result of block execution that may include special control flow commands
     * such as `continue` or `break`.
     *
     * ### Possible Values
     * - `ControlSignal::None`: Indicates that block execution completed normally
     *   without altering control flow.
     * - `ControlSignal::Continue`: Signals that an ongoing loop iteration should
     *   be interrupted, and execution should skip to the next iteration.
     * - `ControlSignal::Break`: Signals an immediate termination of the nearest loop structure.
     *
     * This field is integral to ensuring that the interpreter handles advanced
     * control flow scenarios appropriately.
     */
    ControlSignal m_controlSignal = ControlSignal::None;

    /**
     * A mapping of variable names to their corresponding integer values.
     *
     * This member variable stores the runtime state of variables used during
     * interpretation. Each entry in the map associates a variable name (as a string)
     * with its current value (as an unsigned 32-bit integer). The map is used
     * for variable lookups, assignments, and related operations during program execution.
     */
    std::map<std::string, VariableData> m_variables;

    /**
     * A shared pointer to the internal execution stack of the interpreter.
     *
     * This stack is used to manage the state during the interpretation process,
     * such as maintaining the function call hierarchy, storing intermediate values,
     * or handling control flows. Ownership of the stack is shared, ensuring proper
     * memory management across components.
     */
    Stack m_stack;


    /**
     * A collection of Token objects that represent the sequence of instructions
     * or data to be processed by the interpreter.
     *
     * This vector serves as the primary storage for the tokens that guide the
     * behavior of the interpretation or execution process. It maintains the
     * order of tokens as they are processed during interpretation.
     */
    std::vector<Token> m_tokens;


    /**
     * A shared pointer to a Memory object used for managing and accessing
     * the memory operations within the Interpreter.
     *
     * This member variable provides shared ownership of the Memory instance,
     * ensuring that it remains accessible and valid for the lifespan of any
     * component that references it. The Memory object handles storage and
     * retrieval of data required for the execution process.
     */
    Memory m_memory;


    /**
     * @var m_words
     * @brief Stores the dictionary of words and their definitions for the interpreter.
     *
     * The `m_words` variable is a mapping structure that associates string keys
     * (representing word tokens) with their respective definitions of type `WordDef`.
     * It serves as the core repository for managing the words and their corresponding
     * behaviors or meanings within the interpreter.
     *
     * This dictionary is used during interpretation to retrieve and execute the
     * functionality or behavior associated with a particular word token.
     *
     * ### Key Characteristics
     * - The key represents the name of a word as a `std::string`.
     * - The value, of type `WordDef`, contains the definition and implementation details.
     * - Efficient lookup provided by the `std::unordered_map` ensures quick access to word definitions, even in large collections.
     *
     * ### Usage
     * The variable is central to interpreting and executing user input or
     * pre-defined scripts. Words that are not found in this map may result in errors
     * indicating invalid or unrecognized tokens.
     *
     * ### Related Components
     * @see WordDef
     */
    std::unordered_map<std::string, WordDef> m_words;

    /**
     * Tracks the next available memory address for allocation.
     *
     * This member variable stores the address to allocate the next block of memory.
     * It is used to ensure that each allocation operation results in a unique and
     * sequential memory address.
     */
    uint32_t m_nextAvailableMemoryAddress = 0;

    /**
     * Represents the current position or index within a sequence of elements.
     *
     * This member variable is used to track the progress of parsing or iterating
     * through a collection, such as a series of tokens or data. It is initialized
     * to 0 by default, reflecting the starting position.
     */
    size_t m_pos = 0;

    /**
     * @brief Tracks the absolute character index of the most recent tokenizer or interpreter error.
     *
     * This field stores a source-position offset into `m_source` that corresponds
     * to where a parsing or runtime error occurred. It is used to improve error
     * diagnostics by allowing the system to reconstruct the exact line and column
     * later, even if the internal iteration state has already moved forward.
     *
     * A value of `static_cast<size_t>(-1)` indicates that no error position
     * has been recorded yet.
     *
     * Typical cases where this value is set include:
     *  - Unterminated string literals
     *  - Unexpected/misplaced keywords (`else`, `endif`, `while`, `break`, etc.)
     *  - Unknown or invalid characters
     *  - Tokenization or execution failures requiring contextual error output
     *
     * @note This does not always equal `m_pos`. It may be set explicitly
     *       at the moment an error is detected, especially when error
     *       handling occurs after token consumption continues.
     *
     * @see m_source
     * @see printErrorContext()
     */
    size_t m_errorPos = static_cast<size_t>(-1);


    /**
     * @var std::vector<size_t> m_arrayMarks
     * @brief Stores indices used to mark positions within nested array structures during execution.
     *
     * The `m_arrayMarks` vector is primarily utilized by the interpreter for keeping track of
     * array boundary positions or levels within nested arrays. This ensures the interpreter
     * can correctly parse and execute expressions involving multi-dimensional or deeply
     * nested array constructs.
     *
     * ### Functional Context
     * - Entries in this vector typically correspond to specific depths or frames within
     *   array parsing or evaluation.
     * - It ensures accurate restoration or maintenance of contextual array states
     *   during recursive or iterative operations.
     *
     * ### Internal Notes
     * Modifications to `m_arrayMarks` are typically done by methods that handle
     * array parsing or AST execution in the interpreter's control flow.
     */
    std::vector<size_t> m_arrayMarks;
    /**
     * @variable m_structMarks
     * @brief Tracks structural markers during code execution.
     *
     * The `m_structMarks` variable is a collection of indices utilized internally
     * by the interpreter to manage and reference positions of structural constructs
     * in the source code. These constructs may include elements such as loop boundaries,
     * conditional branches, or block delimiters.
     *
     * This variable is primarily used to ensure correct interpretation of the
     * source code by maintaining a mapping of critical structural locations,
     * enabling the interpreter to navigate and process complex control flow patterns.
     *
     * ### Purpose
     * - Assists with the management of nested or hierarchical structures in the code.
     * - Supports execution by providing quick access to structural locations.
     *
     * ### Implementation Details
     * - Stored as a `std::vector` of `size_t` to efficiently represent and index code positions.
     * - Updated dynamically during code parsing and execution processes.
     *
     * ### Related Functions
     * @see Interpreter::parseStructure()
     * @see Interpreter::executeBlock()
     */
    std::vector<size_t> m_structMarks;

    // Call stack frames
    std::vector<Frame> m_frames;

    std::unordered_map<std::string, NativeWord> m_nativeWords;
};

#endif //LEXER_H
