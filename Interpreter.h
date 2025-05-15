#ifndef LEXER_H
#define LEXER_H
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Stack.h"
#include "tokenizer.h"

using StackValue = std::variant<int, std::string>;

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
    explicit Interpreter(std::vector<Token> tokens, std::shared_ptr<Stack> stack);


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
     * Executes the 'push' operation, pushing a value onto the stack.
     *
     * This function retrieves the value associated with the current token
     * and pushes it onto the stack.
     */
    void executePush(const StackValue &value) const;

    /**
     * Executes the "over" operation on the stack.
     *
     * This method duplicates the second-to-top value on the stack and pushes it
     * back onto the stack without altering the original positions of other values.
     * If the stack contains fewer than two elements, an exception is thrown.
     *
     * @throws std::runtime_error If the stack contains fewer than two elements, a stack underflow error is raised.
     */
    void executeOver() const;

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
    void executeNip() const;

    /**
     * Executes the TUCK operation on the stack.
     *
     * The TUCK operation duplicates the top value of the stack and places it two
     * positions down, rearranging the stack order. This method requires at least
     * two elements on the stack to perform the operation; otherwise, an exception
     * is thrown.
     *
     * @throws std::runtime_error Thrown if there are fewer than two elements
     * present on the stack when the method is called.
     */
    void executeTuck() const;


    /**
     * Executes the 'print' operation, printing the value on the top of the stack.
     *
     * This function retrieves the value from the top of the stack and prints it
     * to the standard output.
     */
    void executePrint() const;

    /**
     * Executes a binary arithmetic operation.
     *
     * This function performs the arithmetic operation specified by the 'tokenType'
     * on the top two values of the stack.
     *
     * @param tokenType The type of binary operation to execute (e.g., ADD, SUB, MUL, DIV, MOD).
     */
    void executeBinary(TokenType tokenType) const;


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
    void executeLogical(TokenType tokenType) const;


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
    void executeZeroCheck() const;


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
     * Prints the value held by a std::variant to the standard output stream (std::cout).
     *
     * This function uses std::visit to determine the actual type of the value
     * stored within the variant and then prints that value accordingly. It handles
     * both 'int' and 'std::string' types.
     *
     * @param value A const reference to the std::variant whose value is to be printed.
     */
    static void printVariant(const std::variant<int, std::string>& value);

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
    static bool isOfType(std::variant<int, std::string> &value);


    /**
     * Retrieves the integer value from a variant, or throws an exception if the variant
     * does not hold an integer.
     *
     * @param value A const reference to the std::variant from which to retrieve the
     * integer value.
     * @return The integer value held within the variant.
     * @throws std::runtime_error if the variant does not hold an integer value.
     */
    static int GetIntOrThrow(const std::variant<int, std::string>& value);

    /**
     * Retrieves the string value from a variant, or throws an exception if the variant
     * does not hold a string.
     *
     * @param value A const reference to the std::variant from which to retrieve the
     * string value.
     * @return The string value held within the variant.
     * @throws std::runtime_error if the variant does not hold a string value.
     */
    static std::string GetStringOrThrow(const std::variant<int, std::string>& value);

private:
    /**
     * A shared pointer to the internal execution stack of the interpreter.
     *
     * This stack is used to manage the state during the interpretation process,
     * such as maintaining the function call hierarchy, storing intermediate values,
     * or handling control flows. Ownership of the stack is shared, ensuring proper
     * memory management across components.
     */
    std::shared_ptr<Stack> m_stack;


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
     * Represents the current position or index within a sequence of elements.
     *
     * This member variable is used to track the progress of parsing or iterating
     * through a collection, such as a series of tokens or data. It is initialized
     * to 0 by default, reflecting the starting position.
     */
    size_t m_pos = 0;
};

#endif //LEXER_H
