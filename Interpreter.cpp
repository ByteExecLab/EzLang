#include "Interpreter.h"

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <variant>

using StackValue = std::variant<int, std::string>;

Interpreter::Interpreter(std::vector<Token> tokens, std::shared_ptr<Stack> stack)
    : m_stack(std::move(stack)), m_tokens(std::move(tokens)), m_pos(0) {

    if (!m_stack) {
        m_stack = std::make_shared<Stack>();
    }

    executionMap = {
        // Stack operations
        {TokenType::DUP, [this]() { m_stack->dup(); }},
        {TokenType::POP, [this]() { m_stack->pop(); }},
        {TokenType::PEEK, [this]() { m_stack->peek(); }},
        {TokenType::SWAP, [this]() { m_stack->swap(); }},

        // Binary operations
        {TokenType::ADD, [this]() { executeBinary(TokenType::ADD); }},
        {TokenType::SUB, [this]() { executeBinary(TokenType::SUB); }},
        {TokenType::MUL, [this]() { executeBinary(TokenType::MUL); }},
        {TokenType::DIV, [this]() { executeBinary(TokenType::DIV); }},
        {TokenType::MOD, [this]() { executeBinary(TokenType::MOD); }},

        // Logical Operators
        {TokenType::EQUALS, [this]() { executeLogical(TokenType::EQUALS); }},
        {TokenType::LESS_THAN, [this]() { executeLogical(TokenType::LESS_THAN); }},
        {TokenType::GREATER_THAN, [this]() { executeLogical(TokenType::GREATER_THAN); }},
        {TokenType::LESS_THAN_EQUALS, [this]() { executeLogical(TokenType::LESS_THAN_EQUALS); }},
        {TokenType::GREATER_THAN_EQUALS, [this]() { executeLogical(TokenType::GREATER_THAN_EQUALS); }},
        {TokenType::NOT_EQUALS, [this]() { executeLogical(TokenType::NOT_EQUALS); }},
        {TokenType::ZERO_CHECK, [this]() { executeZeroCheck(); }},

        // Control flow
        {TokenType::IF, [this]() { executeIf(); }},

            // Utils
        {TokenType::PRINT, [this]() { executePrint(); }},
    };
}


/**
 * Prints the value held by a std::variant to the standard output stream (std::cout).
 *
 * This function uses std::visit to determine the actual type of the value
 * stored within the variant and then prints that value accordingly.  It handles
 * both 'int' and 'std::string' types and could be extended to handle other
 * types added to the variant.
 *
 * @param value A const reference to the std::variant whose value is to be printed.
 * The variant is passed by const reference to avoid unnecessary copying
 * and to ensure the function does not modify the original variant.
 */
void Interpreter::printVariant(const std::variant<int, std::string>& value) {
    std::visit([](auto&& arg) { std::cout << arg << std::endl; }, value);
}


/**
 * Checks whether a std::variant holds a value of a specific type.
 *
 * This function is a template, allowing it to check if a variant holds
 * any of the types it was declared to hold. It uses std::holds_alternative
 * to perform the type check.
 *
 * @tparam T The type to check for within the variant.  This is a template
 * parameter, so the caller specifies the type they are interested in.
 * @tparam Types A template parameter pack representing the other types that
 * the std::variant can potentially hold.  This is deduced from the
 * variant itself.
 * @param value A const reference to the std::variant to check.  Passing by
 * const reference avoids unnecessary copying and prevents
 * modification of the original variant.
 * @return true if the variant holds a value of type T, false otherwise.
 */
template<typename T>
bool Interpreter::isOfType(StackValue &value) {
    return std::holds_alternative<T>(value);
}


/**
 * Retrieves the integer value from a variant or throws an exception if the variant
 * does not hold an integer.
 *
 * This function attempts to extract the integer value from the provided variant.
 * If the variant holds an integer, that integer is returned. If the variant
 * holds a different type (e.g., a string), a std::runtime_error exception is
 * thrown with a specific error message. This ensures that the program handles
 * unexpected types gracefully.
 *
 * @param value A const reference to the std::variant from which to retrieve the
 * integer value.  Passing by const reference avoids unnecessary copying
 * and prevents modification of the original variant.
 * @return The integer value held within the variant.
 * @throws std::runtime_error if the variant does not hold an integer value.
 */
int Interpreter::GetIntOrThrow(const StackValue &value) {
    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value);
    }
    throw std::runtime_error("[ERROR]: Expected int value on stack");
}

/**
 * Retrieves the string value from a variant or throws an exception if the variant
 * does not hold a string.
 *
 * This function attempts to extract the string value from the provided variant.
 * If the variant holds a string, that string is returned. If the variant
 * holds a different type (e.g., an integer), a std::runtime_error exception is
 * thrown with a specific error message. This ensures that the program handles
 * unexpected types gracefully.
 *
 * @param value A const reference to the std::variant from which to retrieve the
 * string value. Passing by const reference avoids unnecessary copying
 * and prevents modification of the original variant.
 * @return The string value held within the variant.
 * @throws std::runtime_error if the variant does not hold a string value.
 */
std::string Interpreter::GetStringOrThrow(const StackValue &value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    throw std::runtime_error("[ERROR]: Expected string value on stack");
}

/**
 * Retrieves the current token and advances the interpreter's position to the next token.
 *
 * This function retrieves the token at the interpreter's current position
 * within the token vector.  It then increments the position, effectively
 * "consuming" the token.  If the current position is already at or beyond the
 * end of the token vector, an exception is thrown, indicating an unexpected
 * end of input.
 *
 * @return The token at the interpreter's current position before it was advanced.
 * @throws std::runtime_error if the current position is at or beyond the end of
 * the token vector, indicating an attempt to read past the end of the input.
 */
Token Interpreter::consume() {
    if (m_pos >= m_tokens.size()) {
        throw std::runtime_error("[ERROR]: Unexpected end of tokens");
    }
    return m_tokens.at(m_pos++);
}

/**
 * Peeks at a token ahead of the current position without consuming it.
 *
 * This function allows the interpreter to look at a token in the token stream
 * without advancing its current position.  It's useful for lookahead operations,
 * such as checking the next token to determine how to handle the current one.
 *
 * @param offset The offset from the current position.  An offset of 0 returns
 * the current token, an offset of 1 returns the next token, and
 * so on.  Must be a non-negative value.
 * @return  A std::optional<Token> representing the token at the specified
 * offset.  If the offset is within the bounds of the token vector,
 * the function returns the token wrapped in a std::optional.
 * If the offset is beyond the end of the token vector, the function
 * returns std::nullptr to indicate that there is no token at that
 * position.
 */
std::optional<Token> Interpreter::peek(size_t offset) {
    if (m_pos + offset >= m_tokens.size()) {
        return std::nullopt;
    }

    return m_tokens.at(m_pos + offset);
}

/**
 * Executes the 'print' operation, which prints the value on the top of the stack
 * to the standard output (stdout).
 *
 * This function retrieves the value from the top of the stack without removing it
 * (using `peek`), and then prints that value to the console using the
 * `printVariant` function. It assumes that the current token is the 'print'
 * instruction.  It then consumes the 'print' token.
 *
 */
void Interpreter::executePrint() const {
    const auto print_value = m_stack->peek();
    printVariant(print_value);
}

/**
 * Executes a 'push' operation, pushing a value onto the stack.
 *
 * This function retrieves the value associated with the current token
 * (which is assumed to be a value to be pushed). It then consumes the
 * current token.  If the next token is also a 'PUSH' token, it
 * assumes that the current token's value should be pushed onto the
 * stack and then consumes the next 'PUSH' token.
 *
 */
void Interpreter::executePush(const StackValue &value) const {
    m_stack->push(value);
}

/**
 * Executes a binary arithmetic operation (addition, subtraction, multiplication,
 * division, or modulo) on the top two values on the stack.
 *
 * This function performs the specified arithmetic operation on the two values
 * at the top of the stack. It first consumes the operator token. It then checks
 * for stack underflow (if there are fewer than two values on the stack).  It
 * pops the two operands from the stack, performs the operation, and pushes the
 * result back onto the stack.  It uses the GetIntOrThrow helper function to
 * ensure that the operands are integers.
 *
 * @param tokenType The TokenType representing the binary operation to perform.
 * Must be one of TokenType::ADD, TokenType::SUB, TokenType::MUL,
 * TokenType::DIV, or TokenType::MOD.
 *
 */
void Interpreter::executeBinary(const TokenType tokenType) const {
    if (m_stack->size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for binary operation");
    }

    StackValue l_value = m_stack->pop();
    StackValue r_value = m_stack->pop();
    StackValue result;

    switch (tokenType) {
        case TokenType::ADD: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                result = GetStringOrThrow(l_value) + GetStringOrThrow(r_value);
            }
            else if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                result = int_a + int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for + operation");
            } break;
        };
        case TokenType::SUB: {
            if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                result = int_a - int_b;
            } break;
        }
        case TokenType::MUL: {
            if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                result = int_a * int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for * operation");
            } break;
        }
        case TokenType::DIV: {
            if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                // Catch division by zero
                if (int_b == 0) {
                    throw std::runtime_error("[ERROR]: Division by zero");
                }
                result = int_a / int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for / operation");
            } break;
        }
        case TokenType::MOD: {
            if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                // Catch division by zero
                if (int_b == 0) {
                    throw std::runtime_error("[ERROR]: Modulo by zero");
                }
                result = int_a % int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for % operation");
            } break;
        }
        default: {
            //
        };
    }

    m_stack->push(result);
}

/**
 * Executes a logical operation on the top two values on the stack.
 *
 * This function pops two values from the stack, performs the specified logical
 * operation (equality, less than, greater than, etc.), and pushes the boolean
 * result (represented as 1 for true, 0 for false) back onto the stack.
 *
 * @param tokenType The TokenType representing the logical operation to perform.
 * Must be one of TokenType::EQUALS, TokenType::LESS_THAN,
 * TokenType::GREATER_THAN, TokenType::LESS_THAN_EQUALS,
 * TokenType::GREATER_THAN_EQUALS, or TokenType::NOT_EQUALS.
 *
 * @throws std::runtime_error if the stack contains fewer th two elements.
 */
void Interpreter::executeLogical(const TokenType tokenType) const {
    if (m_stack->size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for logical operation");
    }

    StackValue l_value = m_stack->pop();
    StackValue r_value = m_stack->pop();

    StackValue result;

    switch (tokenType) {
        case TokenType::EQUALS: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                result = GetStringOrThrow(l_value) == GetStringOrThrow(r_value);
            }
            else if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a == int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for = operation");
            } break;
        }
        case TokenType::LESS_THAN: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a < int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for < operation");
            } break;
        };
        case TokenType::GREATER_THAN: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a > int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for > operation");
            } break;
        }
        case TokenType::LESS_THAN_EQUALS: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a <= int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for <= operation");
            } break;
        }
        case TokenType::GREATER_THAN_EQUALS: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a >= int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for >= operation");
            } break;
        }
        case TokenType::NOT_EQUALS: {
            if (isOfType<std::string>(l_value) && isOfType<std::string>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);

                result = int_a != int_b;
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for != operation");
            } break;
        }
        default: {
            //
        };
    }

    m_stack->push(result);
}

/*
 * Executes an IF control flow statement.
 *
 * This function pops a condition from the stacks. If the condition is true (non-zero),
 * it executes the code within the IF block.  It parses the tokens to find the
 * boundaries of the IF block (and optionally an ELSE block) and then creates
 * a new Interpreter to execute the appropriate branch.
 */
void Interpreter::executeIf() {
    consume(); // consume IF

    // Checking if the stack is not empty
    if (m_stack->empty()) {
        throw std::runtime_error("Stack underflow: IF condition");
    }

    // POP condition from the stacks
    const auto condition = m_stack->pop();
    bool conditionIsTrue = false;
    if (std::holds_alternative<int>(condition)) {
        conditionIsTrue = (std::get<int>(condition) != 0);
    }

    // Collect tokens
    std::vector<Token> ifBranch;
    std::vector<Token> elseBranch;
    bool inElseBlock = false;

    //read all the tokens inside the if and else branches
    while (m_pos < m_tokens.size() && (m_tokens[m_pos].type != TokenType::END)) {
        if (m_tokens[m_pos].type == TokenType::ELSE) {
            inElseBlock = true;
            consume(); // Consume else
        }
        else if (inElseBlock) {
            elseBranch.push_back(consume());
        }
        else {
            ifBranch.push_back(consume());
        }
    }
    if (m_pos >= m_tokens.size() || m_tokens[m_pos].type != TokenType::END) {
        throw std::runtime_error("Expected ENDIF after IF-ELSE block");
    }

    // consume(); // Consume ENDIF

    if (conditionIsTrue) {
        // Execute the 'if' branch
        Interpreter ifInterpreter(ifBranch, m_stack); // Create a new interpreter for the if branch
        ifInterpreter.executionMap = executionMap;
        ifInterpreter.execute();
    } else if (inElseBlock) {
        Interpreter elseInterpreter(elseBranch, m_stack); // Create a new interpreter for the else branch
        elseInterpreter.executionMap = executionMap;
        elseInterpreter.execute();
    }
}


/**
 * Executes the ZERO_CHECK operation.
 *
 * This function pops the top value from the stack, checks if it is equal to zero,
 * and pushes the boolean result (1 for true, 0 for false) back onto the stack.
 *
 * @throws std::runtime_error if the stack is empty.
 */
void Interpreter::executeZeroCheck() const {
    if (m_stack->empty()) {
        throw std::runtime_error("[ERROR]: Stack underflow for zero check");
    }

    const StackValue value = m_stack->pop();
    const int int_a = GetIntOrThrow(value);
    m_stack->push(int_a == 0);
}


/**
 * Executes the sequence of tokens provided to the Interpreter.
 *
 * This function is the main control loop of the interpreter. It iterates
 * through the vector of tokens, fetching each token and dispatching
 * to the appropriate function to handle the token's associated operation.
 * The dispatching is done using a map that associates each TokenType
 * with its corresponding execution function.  This allows for a clean
 * and extensible way to handle different operations.
 *
 * The execution loop continues until all tokens in the input have been
 * processed.  Error handling is included to catch and report any runtime
 * exceptions that occur during the execution of a token (e.g., stack
 * underflow, invalid operations).
 *
 */
void Interpreter::execute() {
    while (m_pos < m_tokens.size()) {
        const auto&[type, value] = m_tokens[m_pos];
        try {
            // Check for literals first
            if (type == TokenType::INT_LITERAL || type == TokenType::STR_LITERAL) {
                executePush(value);
                consume(); // Consume the literal
            }
            else {
                // Use the map to call the appropriate function.
                if (auto it = executionMap.find(type); it != executionMap.end()) {
                    it->second(); // Call the function
                    consume();
                }
                else {
                    throw std::runtime_error("Unknown token type: " + tokenTypeToString(type));
                }
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "[ERROR]: " << e.what() << std::endl;
            std::cerr << "m_pos: " << m_pos << ", m_tokens.size(): " << m_tokens.size() << std::endl;
            std::cerr << "Token type: " << tokenTypeToString(type) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}



