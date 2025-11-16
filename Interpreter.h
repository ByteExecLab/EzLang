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
    explicit Interpreter(std::vector<Token> tokens, Stack stack);


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

    bool executeBlock(const std::vector<Token>& block);

    static bool isTruly(const StackValue& value);

    std::vector<Token> collectUntil(TokenType endType);
    std::pair<std::vector<Token>, std::vector<Token>> collectIfElseEndif();
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

    void executeTrace();

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

    static double toDouble(const StackValue& v);

    static bool bothInt(const StackValue& a, const StackValue& b);

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
};

#endif //LEXER_H
