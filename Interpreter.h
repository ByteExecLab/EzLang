#ifndef LEXER_H
#define LEXER_H
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Memory.h"
#include "Stack.h"
#include "tokenizer.h"

/**
 * @enum ControlSignal
 * @brief Indicates non-local control flow during block execution.
 *
 * The `ControlSignal` enum is used internally by the interpreter to manage
 * structured control flow inside looping constructs. Certain tokens such
 * as `continue` and `break` do not behave like normal instructions; instead,
 * they alter execution flow of the nearest active loop.
 *
 * When a block (such as the body of a `while` loop or an `if` branch)
 * is executed via `executeBlock(...)`, it may return a `ControlSignal`
 * to notify the caller of special flow redirection.
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
     */
    explicit Interpreter(std::vector<Token> tokens, Stack stack, std::string source);


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

    static void validateBlocks(const std::vector<Token>& tokens);

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

    void executeReturn();
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

    void executeAnd();

    void executeOr();

    void executeNot();

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

    ControlSignal m_controlSignal = ControlSignal::None;
    /**
     * A mapping of variable names to their corresponding integer values.
     *
     * This member variable stores the runtime state of variables used during
     * interpretation. Each entry in the map associates a variable name (as a string)
     * with its current value (as an unsigned 32-bit integer). The map is used
     * for variable lookups, assignments, and related operations during program execution.
     */
    std::map<std::string, uint32_t> m_variables;

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

    // User defined words: name -> token body
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
};

#endif //LEXER_H
