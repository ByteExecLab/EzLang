#include "Interpreter.h"

#include <functional>
#include <iostream>
#include <map>
#include <utility>
#include <variant>

Interpreter::Interpreter(std::vector<Token> tokens, Stack stack)
    : m_stack(std::move(stack)), m_tokens(std::move(tokens)), m_memory(1024) {

    executionMap = {
        // Stack operations
        // TODO: Move below operations to stack class?
        {
            TokenType::DUP, [this]() {
                consume(TokenType::DUP, "[Error:dup]: Expected DUP operation");
                m_stack.dup();
            }
        },
        {
            TokenType::SWAP, [this]() {
                consume(TokenType::SWAP, "[Error:swap]: Expected SWAP operation");
                m_stack.swap();
            }
        },
        {
            TokenType::DROP, [this]() {
                consume(TokenType::DROP, "[Error:drop]: Expected DROP operation");
                m_stack.drop();
            }
        },
        {
            TokenType::TUCK, [this]() {
                consume(TokenType::TUCK, "[Error:tuck]: Expected TUCK operation");
                m_stack.tuck();
            }
        },
        {
            TokenType::OVER, [this]() {
                consume(TokenType::OVER, "[Error:over]: Expected OVER operation");
                m_stack.over();
            }
        },
        {TokenType::NIP, [this] { executeNip(); }},

        // Binary operations
        {TokenType::ADD, [this]() { executeBinary(TokenType::ADD); }},
        {TokenType::SUB, [this]() { executeBinary(TokenType::SUB); }},
        {TokenType::MUL, [this]() { executeBinary(TokenType::MUL); }},
        {TokenType::DIV, [this]() { executeBinary(TokenType::DIV); }},
        {TokenType::MOD, [this]() { executeBinary(TokenType::MOD); }},

        // Variables
        {TokenType::CONST, [this]() { executeDefineVariable(); }},
        {
            TokenType::IDENTIFIER, [this] {
                if (peek(1)->type == TokenType::LOAD_VARIABLE) {
                    executeLoadVariable();
                } else if (peek(1)->type == TokenType::STORE_VARIABLE) {
                    executeStoreVariable();
                }
            }
        },

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
        {TokenType::WHILE, [this] { executeWhile(); }},
        {TokenType::CONTINUE, [this] { executeContinue(); }},

        // Utils
        {TokenType::PRINT, [this]() { executePrint(); }},
        {TokenType::TRACE, [this] { executeTrace(); }}
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
void Interpreter::printVariant(const std::variant<int, double, std::string>& value) {
    std::visit([](auto&& arg) { std::cout << arg << std::endl; }, value);
}

/**
 * Retrieves the current token being processed in the token sequence.
 *
 * This function accesses a specific token from the stored sequence
 * of tokens based on the current position index. It ensures the return
 * of the token at the index specified by the internal position tracker.
 * The function does not modify the state of the object and guarantees
 * a valid token is returned as it uses bounds-checked access.
 *
 * @return The current token from the sequence, accessed at the position
 * determined by the internal index tracker.
 */
Token Interpreter::getCurrentToken() const {
    return m_tokens.at(m_pos);
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
 * The std::variant can potentially hold.  This is deduced from the
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
 * holds a different type (e.g., an integer), an std::runtime_error exception is
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

Token Interpreter::consume(const TokenType tokenType, const std::string &errorMessage) {
    if (m_pos >= m_tokens.size()) {
        throw std::runtime_error("Unexpected end of tokens");
    }

    const Token &token = m_tokens.at(m_pos);
    if (token.type != tokenType) {
        throw std::runtime_error(errorMessage);
    }

    m_pos++;
    return token;
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
void Interpreter::executePrint() {
    consume(TokenType::PRINT, "[ERROR]: Expected PRINT");
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack underflow for print operation");
    }

    printVariant(m_stack.pop());
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
void Interpreter::executePush(const StackValue &value) {
    m_stack.push(value);
}

/**
 * Executes a string literal operation in the interpreter.
 *
 * This function processes a token of type `TokenType::STR_LITERAL`, ensuring
 * its presence in the input stream. Upon successful consumption, the function
 * extracts the literal value and pushes it onto the interpreter's stack
 * for further execution or evaluation.
 *
 * Any absence of a string literal token in the expected position triggers
 * an error with the provided message, halting execution.
 */
void Interpreter::executeStrLiteral() {
    const auto [type, value] =
        consume(TokenType::STR_LITERAL, "[ERROR]: Expected string literal");
    m_stack.push(value);
}

/**
 * Executes the processing of an integer literal in the interpreter.
 *
 * This function consumes a token of type TokenType::INT_LITERAL, ensuring that it matches
 * the expected type. If the token does not match, an error message is provided. The value
 * of the integer literal is then pushed onto the interpreter's stack for further processing.
 *
 * This operation is critical for handling integer literals during interpretation of code,
 * enabling later operations to access or manipulate the stored value.
 *
 * The function interacts closely with the token stream by consuming the token and also
 * the stack where the value is stored after processing.
 */
void Interpreter::executeIntLiteral() {
    const auto [type, value] = consume(TokenType::INT_LITERAL, "[ERROR]: Expected integer literal");
    m_stack.push(value);
}

#include <iomanip> // for std::setw, std::left

void Interpreter::executeTrace() {
    consume(TokenType::TRACE, "Expected Trace");

    std::cout << "\n[TRACE] Current Stack State:\n";
    std::cout << "-------------------------------------\n";
    std::cout << std::left << std::setw(6) << "Index"
              << std::setw(10) << "Type"
              << "Value\n";
    std::cout << "-------------------------------------\n";

    const auto values = m_stack.getContents(); // safer than using pop()

    for (size_t i = 0; i < values.size(); ++i) {
        const auto& val = values[i];
        std::cout << std::left << std::setw(6) << i;

        std::visit([]<typename T0>(T0&& v) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, int>) {
                std::cout << std::setw(10) << "int" << v;
            } else if constexpr (std::is_same_v<T, std::string>) {
                std::cout << std::setw(10) << "string" << '"' << v << '"';
            }
        }, val);

        std::cout << '\n';
    }

    if (values.empty()) {
        std::cout << "[empty stack]\n";
    }

    std::cout << "-------------------------------------\n";
    std::cout << "Top of stack is at index: " << (values.empty() ? 0 : values.size() - 1) << "\n\n";
}

void Interpreter::executeContinue() {
    consume(TokenType::CONTINUE, "[ERROR]: Expected CONTINUE");
}


/**
 * Executes the "over" operation on the stack managed by the interpreter.
 *
 * The "over" operation duplicates the second-to-top value on the stack,
 * ensuring that the current top value remains in place. This function
 * will throw a runtime error if there are fewer than two elements
 * on the stack, as the operation cannot be completed in such cases.
 *
 * @throws std::runtime_error If the stack contains fewer than two elements,
 * indicating a stack underflow condition.
 */
void Interpreter::executeOver() {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for over operation");
    }

    consume(TokenType::OVER, "[ERROR]: Expected over operation");

    const StackValue top_value = m_stack.pop();
    const StackValue second_value = m_stack.peek();

    m_stack.push(top_value);
    m_stack.push(second_value);
}

/**
 * Executes the "nip" operation on the stack, removing the second-to-top value
 * while keeping the top value.
 *
 * This function operates on the stack referenced by the `Interpreter`. It first
 * verifies that the stack has at least two elements; if not, a runtime_error
 * is thrown to indicate stack underflow. The top value is preserved, the second
 * value is removed, and then the top value is pushed back onto the stack.
 *
 * @throws std::runtime_error Thrown if the stack contains fewer than two elements.
 */
void Interpreter::executeNip() {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for nip operation");
    }

    consume(TokenType::NIP, "Expected NIP operation");

    // Get first value
    const StackValue top_value = m_stack.pop();

    // Remove second value
    m_stack.pop();

    // Push the first value back
    m_stack.push(top_value);
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
void Interpreter::executeBinary(const TokenType tokenType) {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for binary operation");
    }

    StackValue l_value = m_stack.pop();
    StackValue r_value = m_stack.pop();
    StackValue result;

    switch (tokenType) {
        case TokenType::ADD: {
            // Consume ADD token
            consume(TokenType::ADD, "[ERROR]: Expected ADD operation");

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
            // Consume SUB token
            consume(TokenType::SUB, "[ERROR]: Expected SUB operation");

            if (isOfType<int>(l_value) && isOfType<int>(r_value)) {
                const int int_a = GetIntOrThrow(l_value);
                const int int_b = GetIntOrThrow(r_value);
                result = int_b - int_a;
            } break;
        }
        case TokenType::MUL: {
            // Consume MUL token
            consume(TokenType::MUL, "[ERROR]: Expected MUL operation");

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
            // Consume DIV token
            consume(TokenType::DIV, "[ERROR]: Expected DIV operation");

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
            // Consume MOD token
            consume(TokenType::MOD, "[ERROR]: Expected MOD operation");

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

    m_stack.push(result);
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
void Interpreter::executeLogical(const TokenType op) {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for logical operation");
    }

    StackValue l_value = m_stack.pop();
    StackValue r_value = m_stack.pop();

    // Helper lambda for numeric comparison
    auto toDouble = [&](const StackValue& v) -> double {
        return std::visit([](auto&& val) -> double {
            using T = std::decay_t<decltype(val)>;

            if constexpr (std::is_same_v<T, int>) {
                return static_cast<double>(val);
            }

            if constexpr (std::is_same_v<T, double>) {
                return val;
            }

            throw std::runtime_error("Expected number for comparison");
        }, v);
    };

    // Helper labda for string comparison
    auto toString = [&](const StackValue& v) -> std::string {
        if (auto s = std::get_if<std::string>(&v)) {
            return *s;
        }

        throw std::runtime_error("Expected string for comparison");
    };

    // Comparison result
    bool result = false;

    switch (op) {
        case TokenType::EQUALS: {
            result = (l_value == r_value);
            break;
        }
        case TokenType::NOT_EQUALS: {
            result = !(l_value == r_value);
            break;
        }
        case TokenType::LESS_THAN: {
            result = toDouble(l_value) < toDouble(r_value);
            break;
        }
        case TokenType::LESS_THAN_EQUALS: {
            result = toDouble(l_value) <= toDouble(r_value);
            break;
        }
        case TokenType::GREATER_THAN: {
            result = toDouble(l_value) > toDouble(r_value);
            break;
        }
        case TokenType::GREATER_THAN_EQUALS: {
            result = toDouble(l_value) >= toDouble(r_value);
            break;
        }
        default: {
            throw std::runtime_error("Unknown logical operator");
        }
    }

    // Push bool as int (1 or 0)
    m_stack.push(result ? 1 : 0);
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
    consume(TokenType::IF, "[ERROR]: Expected IF operation");

    if (m_stack.empty()) {
        throw std::runtime_error("Stack underflow: IF condition");
    }

    // POP condition from the stack
    const StackValue conditionResult = m_stack.pop();
    const bool conditionIsTrue = isTruly(conditionResult);

    // Collect tokens
    auto [ifBranch, elseBranch] = collectIfElseEndif();

    executeBlock(conditionIsTrue ? ifBranch : elseBranch);

}

/**
 * Executes a WHILE-DO loop based on a tokenized input sequence.
 *
 * This method processes a WHILE-DO block by parsing the condition and body tokens from the input sequence,
 * evaluating the condition, and executing the body repeatedly as long as the condition evaluates to true.
 * It handles the separation of tokens into condition and body blocks and ensures the correct flow of execution.
 *
 * - A WHILE-DO block begins with a WHILE token, followed by a condition block, a DO token, a body block,
 *   and ends with an END token.
 * - The condition is evaluated by creating a temporary Interpreter instance to process the condition tokens.
 *   The result of the condition evaluation is retrieved from the stack and checked.
 * - If the condition evaluates to true, another temporary Interpreter instance is used to process the body tokens.
 * - This continues in a loop until the condition evaluates to false.
 *
 * Exceptions:
 * - Throws std::runtime_error if the END token is missing after the WHILE-DO block.
 * - Throws std::runtime_error if a stack underflow occurs when attempting to retrieve the condition result.
 *
 * Preconditions:
 * - The token sequence provided to the Interpreter must contain a valid WHILE-DO block structure.
 * - The execution stack (m_stack) must be initialized and available for performing operations.
 */
void Interpreter::executeWhile() {
    consume(TokenType::WHILE, "[ERROR]: Expected WHILE operation");

    const std::vector<Token> conditionTokens = collectUntil(TokenType::DO);
    consume(TokenType::DO, "[ERROR]: Expected DO after WHILE condition");

    const std::vector<Token> bodyTokens = collectUntil(TokenType::END);
    consume(TokenType::END, "[ERROR]: Expected END after WHILE block");

    while (true) {
        // Save interpreter state
        const auto prevTokens = m_tokens;
        const auto prevPos = m_pos;

        // Set up condition tokens
        m_tokens = conditionTokens;
        m_pos = 0;

        executeBlock(conditionTokens);

        if (m_stack.empty()) {
            throw std::runtime_error("[ERROR]: WHILE condition stack underflow");
        }

        if (!isTruly(m_stack.pop())) {
            break;
        }

        // Set up body tokens
        m_tokens = bodyTokens;
        m_pos = 0;

        if (executeBlock(bodyTokens)) {
            // continue encountered — just iterate again
        }

        // Restore original tokens
        m_tokens = prevTokens;
        m_pos = prevPos;
    }
}


/**
 * Defines a new variable in the interpreter's symbol table.
 *
 * This method processes the token at the current position to define a variable
 * name and assigns it a new memory address within the interpreter's memory model.
 * It validates that the variable has not been previously defined and associates
 * the value at the top of the stack with the allocated memory address. The stack
 * value is then written to memory.
 *
 * The process includes:
 * - Consuming the token stream to process the variable definition.
 * - Validating the presence of an identifier token.
 * - Checking for name collisions with previously defined variables.
 * - Incrementing the internal memory address counter for the next variable.
 * - Popping the stack to retrieve the value and storing it in memory.
 *
 * @throws std::runtime_error If the next token is not an identifier, or if the variable
 * name has already been defined.
 */
void Interpreter::executeDefineVariable() {
    consume(TokenType::CONST, "Expected const before IDENTIFIER"); // Consume CONST token

    if (m_tokens[m_pos].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected IDENTIFIER after CONST");
    }
    const auto variableName = GetStringOrThrow(m_tokens[m_pos].value);
    consume(TokenType::IDENTIFIER, "Expected IDENTIFIER after CONST"); // Consume IDENTIFIER token

    // Check if a variable has already been defined
    if (m_variables.contains(variableName)) {
        throw std::runtime_error("Variable already defined");
    }

    // Assign a memory address to the variable
    const uint32_t address = m_nextAvailableMemoryAddress;
    m_variables[variableName] = address;
    m_nextAvailableMemoryAddress += sizeof(StackValue);

    if (m_pos >= m_tokens.size() || m_tokens[m_pos].type != TokenType::END) {
        throw std::runtime_error("Expected END after identifier in CONST definition");
    }

    if (m_stack.empty()) {
        throw std::runtime_error("Stack underflow for variable definition");
    }

    // POP value from the stack
    const StackValue value = m_stack.pop();
    m_memory.write(address, value);

    consume(TokenType::END, "Expected END after variable declaration"); // Consume END
}

/**
 * Loads a variable value into the stack from memory using its name.
 *
 * This function retrieves a variable by its name from a token. If the variable
 * exists in the variable registry, its memory address is obtained, and its
 * value is read from memory. The value is then pushed onto the stack for further
 * use in the interpreter. If the variable does not exist, an exception is thrown.
 *
 * @throw std::runtime_error If the variable is not defined.
 */
void Interpreter::executeLoadVariable() {
    const auto variableName = std::get<std::string>(m_tokens[m_pos].value);
    consume(TokenType::IDENTIFIER, "Expected IDENTIFIER before @"); // consume IDENTIFIER

    if (m_variables.contains(variableName)) {
        const uint32_t address = m_variables[variableName];
        const StackValue value = m_memory.read(address);
        m_stack.push(value);
    }
    else {
        throw std::runtime_error("Variable not defined");
    }

    consume(TokenType::LOAD_VARIABLE, "Expected @ after IDENTIFIER"); // consume END
}
/**
 * Executes the operation to store a variable into memory.
 *
 * This function retrieves a variable's identifier from the token stream, verifies
 * that the variable exists in the current context, and writes a value from the
 * stack to the memory address associated with the variable. It ensures that the
 * stack and variable store contain the required elements to correctly perform
 * the operation. If a variable is undefined or if the stack is empty, an exception
 * is thrown to indicate the respective error.
 *
 * Exception cases:
 * - Throws std::runtime_error if the stack is empty when attempting to retrieve a value.
 * - Throws std::runtime_error if the variable is not defined in the current context.
 *
 * @throws std::runtime_error If the stack is empty, resulting in a stack underflow,
 * or if the specified variable is not defined.
 *
 * Operation:
 * - Consumes an IDENTIFIER token from the token stream expected before the store operation.
 * - Checks the variable name against the variable map to ensure its existence.
 * - Pops the top value from the stack, then writes the value to the variable's associated memory address.
 * - Consumes the STORE_VARIABLE token to complete the operation.
 */
void Interpreter::executeStoreVariable() {
    if (m_stack.empty()) {
        throw std::runtime_error("Stack underflow for variable store");
    }

    const auto variableName = std::get<std::string>(m_tokens[m_pos].value);
    consume(TokenType::IDENTIFIER, "Expected IDENTIFIER before !. Got: " + tokenTypeToString(m_tokens[m_pos].type)); // consume IDENTIFIER

    // TODO: Check type before storing
    if (m_variables.contains(variableName)) {
        const uint32_t address = m_variables[variableName];
        const StackValue value = m_stack.pop();
        m_memory.write(address, value);
    }
    else {
        throw std::runtime_error("Variable not defined");
    }

    consume(TokenType::STORE_VARIABLE, "Expected ! after IDENTIFIER. Got: " + tokenTypeToString(m_tokens[m_pos].type)); // consume END
}

bool Interpreter::executeBlock(const std::vector<Token> &block) {
    auto oldTokens = m_tokens;
    auto oldPos = m_pos;

    m_tokens = block;
    m_pos = 0;

    while (m_pos < m_tokens.size()) {
        if (const Token& token = m_tokens[m_pos]; token.type == TokenType::CONTINUE) {
            ++m_pos;
            m_tokens = oldTokens;
            m_pos = oldPos;
            return true;  // signal continue
        }

        execute();
    }

    m_tokens = oldTokens;
    m_pos = oldPos;
    return false;
}

bool Interpreter::isTruly(const StackValue &v) {
    return std::visit([]<typename T0>(T0&& value) -> bool {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, int>) {
            return value != 0;
        }

        if constexpr (std::is_same_v<T, double>) {
            return value != 0.0;
        }

         if constexpr (std::is_same_v<T, std::string>) {
             return !value.empty();
         }

        return false;
    }, v);
}

std::vector<Token> Interpreter::collectUntil(const TokenType endType) {
    std::vector<Token> collected;

    while (m_pos < m_tokens.size()) {
        const Token& token = m_tokens[m_pos];

        if (token.type == endType) {
            // Do not consume end token, leave for caller to handle
            break;
        }

        collected.push_back(token);
        ++m_pos;
    }

    if (m_pos >= m_tokens.size()) {
        throw std::runtime_error("[ERROR]: Unexpected end of input. Expected token: " + tokenTypeToString(endType));
    }

    return collected;
}

std::pair<std::vector<Token>, std::vector<Token>> Interpreter::collectIfElseEndif() {
    std::vector<Token> ifBranch;
    std::vector<Token> elseBranch;

    bool inElseBody = false;
    int depth = 1;

    while (m_pos < m_tokens.size()) {
        const Token& token = m_tokens[m_pos];

        if (token.type == TokenType::IF) {
            depth++;
            (inElseBody ? elseBranch : ifBranch).push_back(consume());
        }
        else if (token.type == TokenType::ENDIF) {
            depth--;
            consume(); // Consume ENDIF

            if (depth == 0) { break; }

            (inElseBody ? elseBranch : ifBranch).push_back(consume());
        }
        else if (token.type == TokenType::ELSE && depth == 1) {
            inElseBody = true;
            consume(); // Consume ELSE
        }
        else {
            (inElseBody ? elseBranch : ifBranch).push_back(consume());
        }
    }

    if (depth != 0) {
        throw std::runtime_error("Expected ENDIF");
    }

    return { ifBranch, elseBranch };
}




/**
 * Executes the ZERO_CHECK operation.
 *
 * This function pops the top value from the stack, checks if it is equal to zero,
 * and pushes the boolean result (1 for true, 0 for false) back onto the stack.
 *
 * @throws std::runtime_error if the stack is empty.
 */
void Interpreter::executeZeroCheck() {
    // Consume the ZERO_CHECK token itself
    consume(TokenType::ZERO_CHECK, "[ERROR]: Expected ?");

    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack underflow for zero-check");
    }

    StackValue v = m_stack.pop();

    const bool isZero = std::visit([]<typename T0>(T0&& value) -> bool {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, int>) {
            return value == 0;
        }

        if constexpr (std::is_same_v<T, double>) {
            return value == 0.0;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            return value.empty();
        }
    }, v);

    m_stack.push(isZero ? 1 : 0);
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
            if (type == TokenType::END) {
                throw std::runtime_error("Unexpected END found outside a block");
            }

            if (type == TokenType::INT_LITERAL || type == TokenType::STR_LITERAL || type == TokenType::FLOAT_LITERAL) {
                executePush(value);
                consume();
            }
            else if (auto it = executionMap.find(type); it != executionMap.end()) {
                it->second(); // Call the function
            }
            else {
                throw std::runtime_error("Unknown token type: " + tokenTypeToString(type));
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "\n[ERROR]: Runtime error encountered!" << std::endl;
            std::cerr << "Message      : " << e.what() << std::endl;
            std::cerr << "Current Pos  : " << m_pos << " / " << m_tokens.size() << std::endl;

            if (m_pos < m_tokens.size()) {
                const Token& currentToken = m_tokens[m_pos];
                std::cerr << "Token Type   : " << tokenTypeToString(currentToken.type) << std::endl;

                // Show token value depending on variant type
                std::visit([](auto&& arg) {
                    std::cerr << "Token Value  : " << arg << std::endl;
                }, currentToken.value);
            } else {
                std::cerr << "Token        : <Position beyond token list>" << std::endl;
            }

            std::cerr << "Suggestion   : Check the token and surrounding code near position " << m_pos << std::endl;
            std::cerr << "Execution halted.\n" << std::endl;

            exit(EXIT_FAILURE);
        }
    }
}



