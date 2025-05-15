#ifndef LEXER_H
#define LEXER_H
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
     * @return  An std::optional<Token> representing the token at the specified
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
    void executePush(const std::variant<int, std::string> &value) const;


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


    void executeLogical(TokenType tokenType) const;

    void executeZeroCheck() const;

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
     * Checks if a std::variant holds a value of a specific type.
     *
     * @tparam T The type to check for within the variant.
     * @tparam Types A template parameter pack representing the other types that
     * the std::variant can potentially hold.
     * @param value A const reference to the std::variant to check.
     * @return true if the variant holds a value of type T, false otherwise.
     */
    template<typename T, typename... Types>
    bool isOfType(const std::variant<T, Types...>& value);


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
    std::shared_ptr<Stack> m_stack;
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
};

#endif //LEXER_H
