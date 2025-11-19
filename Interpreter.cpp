#include "Interpreter.h"

#include <functional>
#include <iostream>
#include <map>
#include <utility>
#include <variant>

Interpreter::Interpreter(std::vector<Token> tokens, Stack stack, std::string source)
    : m_stack(std::move(stack)), m_tokens(std::move(tokens)), m_memory(1024), m_source(std::move(source)) {

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
        { TokenType::IDENTIFIER, [this]() { executeIdentifier(); }},

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
        {TokenType::WHILE, [this]() { executeWhile(); }},
        {TokenType::CONTINUE, [this]() { executeContinue(); }},
        {TokenType::BREAK, [this]() { executeBreak(); }},
        {TokenType::RETURN, [this]() { executeReturn(); }},

        // Utils
        {TokenType::PRINT, [this]() { executePrint(); }},
        {TokenType::TRACE, [this]() { executeTrace(); }},
        {TokenType::WORD, [this]() { executeWordDefinition(); }}
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
void Interpreter::printVariant(const StackValue& value) {
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

bool Interpreter::isNumber(const StackValue &v) {
    return std::holds_alternative<int>(v) || std::holds_alternative<double>(v);
}

bool Interpreter::bothInt(const StackValue &a, const StackValue &b) {
    return std::holds_alternative<int>(a) && std::holds_alternative<int>(b);
}

double Interpreter::toDouble(const StackValue &v) {
    if (std::holds_alternative<int>(v)) {
        return static_cast<double>(std::get<int>(v));
    }
    if (std::holds_alternative<double>(v)) {
        return static_cast<double>(std::get<double>(v));
    }

    throw std::runtime_error("[ERROR]: Expected numeric type");
}

void Interpreter::validateBlocks(const std::vector<Token> &tokens) {
     std::vector<BlockFrame> stack;

    auto errorAt = [](const std::string& msg, const Token& tok) {
        throw std::runtime_error(
            msg + " at line " + std::to_string(tok.line) +
            ", column " + std::to_string(tok.column)
        );
    };

    for (const auto & tok : tokens) {
        switch (tok.type) {

            // ----- Block openers -----
            case TokenType::IF:
                stack.push_back({BlockKind::If, tok, false});
                break;

            case TokenType::WHILE:
                stack.push_back({BlockKind::While, tok, false});
                break;

            // If you have a WORD token for `word` definitions:
            case TokenType::WORD:
                stack.push_back({BlockKind::Word, tok, false});
                break;

            // ----- ELSE -----
            case TokenType::ELSE: {
                if (stack.empty() || stack.back().kind != BlockKind::If) {
                    errorAt("[ERROR]: 'else' without matching 'if'", tok);
                }
                if (stack.back().sawElse) {
                    errorAt("[ERROR]: Multiple 'else' clauses for one 'if'", tok);
                }
                stack.back().sawElse = true;
                break;
            }

            // ----- ENDIF -----
            case TokenType::ENDIF: {
                if (stack.empty() || stack.back().kind != BlockKind::If) {
                    errorAt("[ERROR]: 'endif' without matching 'if'", tok);
                }
                stack.pop_back();
                break;
            }

            // ----- DO -----
            case TokenType::DO: {
                if (stack.empty() || stack.back().kind != BlockKind::While) {
                    errorAt("[ERROR]: 'do' without matching 'while'", tok);
                }
                // no push/pop; DO is just a separator inside WHILE
                break;
            }

            // ----- END -----
            case TokenType::END: {
                if (stack.empty()) {
                    // This could be too strict if you use bare `end` for other things.
                    // If that’s the case, either:
                    //   - treat unmatched END as error (strict), or
                    //   - just ignore it (lenient).
                    //
                    // Strict is better long-term, but for now we’ll error:
                    errorAt("[ERROR]: 'end' without matching block", tok);
                }

                BlockKind kind = stack.back().kind;
                if (kind == BlockKind::While || kind == BlockKind::Word) {
                    stack.pop_back();
                } else {
                    errorAt("[ERROR]: 'end' used to close non-loop / non-word block", tok);
                }
                break;
            }

            default:
                break;
        }
    }

    // Unclosed blocks
    if (!stack.empty()) {
        const BlockFrame& unclosed = stack.back();
        std::string kindStr;
        switch (unclosed.kind) {
            case BlockKind::If:    kindStr = "if";    break;
            case BlockKind::While: kindStr = "while"; break;
            case BlockKind::Word:  kindStr = "word";  break;
        }

        throw std::runtime_error(
            "[ERROR]: Unclosed " + kindStr +
            " starting at line " + std::to_string(unclosed.startToken.line) +
            ", column " + std::to_string(unclosed.startToken.column)
        );
    }
}


void Interpreter::printRuntimeErrorContext(size_t line , size_t column) const {
    size_t idx = 0;
    size_t currentLine = 1;

    // Find start of the given line
    while (currentLine < line && idx < m_source.size()) {
        if (m_source[idx] == '\n') {
            currentLine++;
        }
        idx++;
    }

    size_t line_start = idx;
    while (idx < m_source.size() && m_source[idx] != '\n') {
        idx++;
    }
    size_t line_end = idx;

    std::string lineStr = m_source.substr(line_start, line_end - line_start);
    std::cerr << "    " << lineStr << "\n";
    std::cerr << "    ";
    for (size_t i = 1; i < column; ++i) std::cerr << " ";
    std::cerr << "^\n";
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
std::optional<Token> Interpreter::peek(const size_t offset) {
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
    const auto [type, value, line, col] =
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
    const auto [type, value, line, col] = consume(TokenType::INT_LITERAL, "[ERROR]: Expected integer literal");
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

    // Stack: [..., left, right]
    StackValue r_value = m_stack.pop(); // TOP
    StackValue l_value = m_stack.pop(); // Below TOP

    // Result store
    StackValue result;

    auto consumeOp = [&](TokenType t, const char* msg) {
        consume(t, msg);
    };

    switch (tokenType) {
        case TokenType::ADD: {
            // Consume ADD token
            consumeOp(TokenType::ADD, "[ERROR]: Expected ADD operation");

            // String concatenation
            if (std::holds_alternative<std::string>(l_value) && std::holds_alternative<std::string>(r_value)) {
                const auto& a = std::get<std::string>(l_value);
                const auto& b = std::get<std::string>(r_value);
                result = a + b;
            }

            // Numeric addition
            else if (isNumber(l_value) && isNumber(r_value)) {
                if (bothInt(l_value, r_value)) {
                    int a = std::get<int>(l_value);
                    int b = std::get<int>(r_value);
                    result = a + b;
                } else {
                    double a = toDouble(l_value);
                    double b = toDouble(r_value);
                    result = a + b;
                }
            }
            else {
                throw std::runtime_error("[ERROR]: Mismatched types for + operation");
            }
            break;
        };
        case TokenType::SUB: {
            // Consume SUB token
            consumeOp(TokenType::SUB, "[ERROR]: Expected SUB operation");

            if (!isNumber(l_value) && !isNumber(r_value)) {
                throw std::runtime_error("[ERROR]: Mismatched types for - operation");
            }

            if (bothInt(l_value, r_value)) {
                int a = std::get<int>(l_value);
                int b = std::get<int>(r_value);

                // Left - Right
                result = a - b;
            } else {
                double a = toDouble(l_value);
                double b = toDouble(r_value);
                result = a - b;
            }

            break;
        }
        case TokenType::MUL: {
            // Consume MUL token
            consumeOp(TokenType::MUL, "[ERROR]: Expected MUL operation");

            if (!isNumber(l_value) && !isNumber(r_value)) {
                throw std::runtime_error("[ERROR]: Mismatched types for * operation");
            }

            if (bothInt(l_value, r_value)) {
                int a = std::get<int>(l_value);
                int b = std::get<int>(r_value);

                result = a * b;
            } else {
                double a = toDouble(l_value);
                double b = toDouble(r_value);

                result = a * b;
            }

            break;
        }
        case TokenType::DIV: {
            // Consume DIV token
            consumeOp(TokenType::DIV, "[ERROR]: Expected DIV operation");

            if (!isNumber(l_value) && !isNumber(r_value)) {
                throw std::runtime_error("[ERROR]: Mismatched types for / operation");
            }

            double a = toDouble(l_value);
            double b = toDouble(r_value);

            if (b == 0.0) {
                throw std::runtime_error("[ERROR]: Division by zero");
            }

            // Int / Int = Int, else double
            if (bothInt(l_value, r_value)) {
                int ia = std::get<int>(l_value);
                int ib = std::get<int>(r_value);

                result = ia / ib; // Integer division
            } else {
                result = a / b;
            }

            break;;

        }
        case TokenType::MOD: {
            // Consume MOD token
            consumeOp(TokenType::MOD, "[ERROR]: Expected MOD operation");

            // Modulo only for ints
            if (!bothInt(l_value, r_value)) {
                throw std::runtime_error("[ERROR]: Modulo only supported for INT types");
            }

            int a = std::get<int>(l_value);
            int b = std::get<int>(r_value);

            if (b == 0) {
                throw std::runtime_error("[ERROR]: Modulo by zero");
            }

            result = a % b; // left % right
            break;
        }
        default: {
            throw std::runtime_error("[ERROR]: Unknown binary token");
        }
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

    StackValue r_value = m_stack.pop(); //top
    StackValue l_value = m_stack.pop(); // below top

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
            // string == string
            if (std::holds_alternative<std::string>(l_value) && std::holds_alternative<std::string>(r_value)) {
                result = (toString(l_value) == toString(r_value));
            }
            // numeric == numeric
            else {
                result = (toDouble(l_value) == toDouble(r_value));
            }
            consume(TokenType::EQUALS, "[ERROR]: Expected EQUALS operation");
            break;
        }
        case TokenType::NOT_EQUALS: {
            // string == string
            if (std::holds_alternative<std::string>(l_value) && std::holds_alternative<std::string>(r_value)) {
                result = (toString(l_value) != toString(r_value));
            }
            // numeric != numeric
            else {
                result = (toDouble(l_value) != toDouble(r_value));
            }

            consume(TokenType::NOT_EQUALS, "[ERROR]: Expected NOT_EQUALS operation");
            break;
        }
        case TokenType::LESS_THAN: {
            result = (toDouble(l_value) < toDouble(r_value));

            consume(TokenType::LESS_THAN, "[ERROR]: Expected LESS_THAN operation");
            break;
        }
        case TokenType::LESS_THAN_EQUALS: {
            result = toDouble(l_value) <= toDouble(r_value);

            consume(TokenType::LESS_THAN_EQUALS, "[ERROR]: Expected LESS_THAN_EQUALS operation");
            break;
        }
        case TokenType::GREATER_THAN: {
            result = (toDouble(l_value) > toDouble(r_value));

            consume(TokenType::GREATER_THAN, "[ERROR]: Expected GREATER_THAN operation");
            break;
        }
        case TokenType::GREATER_THAN_EQUALS: {
            result = (toDouble(l_value) >= toDouble(r_value));

            consume(TokenType::GREATER_THAN_EQUALS, "[ERROR]: Expected GREATER_THAN_EQUALS operation");
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
    const size_t ifIndex = m_pos; // index of IF

    consume(TokenType::IF, "[ERROR]: Expected IF operation");

    if (m_stack.empty()) {
        throw std::runtime_error("Stack underflow: IF condition");
    }

    // POP condition from the stack
    const StackValue conditionResult = m_stack.pop();
    const bool conditionIsTrue = isTruly(conditionResult);

    // Collect tokens
    auto [ifBranch, elseBranch] = collectIfElseEndif(ifIndex);

    ControlSignal signal = executeBlock(conditionIsTrue ? ifBranch : elseBranch);

    if (signal != ControlSignal::None) {
        m_controlSignal = signal;
    }

    // If inside a loop, caller (executeBlock for the loop body) will see this.
    // If at top level, execute() will treat it as error if it bubbles that far.
    if (signal == ControlSignal::Continue || signal == ControlSignal::Break) {
        // just propagate it upward: do NOT handle here
        // You can store in m_controlSignal if you prefer, but returning via executeBlock is cleaner.
        // So just return; caller’s executeBlock will see it as return value.
        // (No need to do anything else in this function.)
    }

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

    const std::vector<Token> bodyTokens = collectBlockUntilEnd();
    // consume(TokenType::END, "[ERROR]: Expected END after WHILE block");

    // Save outer signal so we don't leak inner loop control outwards
    ControlSignal outerSignal = m_controlSignal;
    m_controlSignal = ControlSignal::None;

    while (true) {
        // Evaluate condition
        ControlSignal condSig = executeBlock(conditionTokens);
        if (condSig == ControlSignal::Return) {
            // RETURN inside condition is weird, but we propagate
            m_controlSignal = ControlSignal::Return;
            return;
        } else if (condSig == ControlSignal::Break || condSig == ControlSignal::Continue) {
            // Using break/continue in condition is invalid → treat as error
            throw std::runtime_error("[ERROR]: 'break' or 'continue' used inside WHILE condition");
        }

        if (m_stack.empty()) {
            throw std::runtime_error("[ERROR]: WHILE condition stack underflow");
        }

        if (!isTruly(m_stack.pop())) {
            break; // Exit loop normally
        }

        // Execute body
        ControlSignal bodySig = executeBlock(bodyTokens);

        if (bodySig == ControlSignal::Continue) {
            // Just restart the loop
            continue;
        }

        if (bodySig == ControlSignal::Break) {
            // Exit loop entirely
            break;
        }

        if (bodySig == ControlSignal::Return) {
            // Propagate RETURN upward, don't consume it here
            m_controlSignal = ControlSignal::Return;
            return;
        }
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

    consume(TokenType::END, "[ERROR]: Expected END after variable declaration"); // Consume END
}

void Interpreter::executeWordDefinition() {
    consume(TokenType::WORD, "Expected 'word'");

    // name
    const Token nameTok = consume(TokenType::IDENTIFIER, "Expected word name after 'word'");
    const std::string name = std::get<std::string>(nameTok.value);

    // optional arity
    int arity = 0;
    if (m_pos < m_tokens.size() && m_tokens[m_pos].type == TokenType::INT_LITERAL) {
        arity = std::get<int>(m_tokens[m_pos].value);
        consume(TokenType::INT_LITERAL, "Expected integer arity after word name");
    }

    // 🔹 This now correctly handles nested while/if/endif/end
    std::vector<Token> body = collectBlockUntilEnd();

    WordDef def;
    def.body  = std::move(body);
    def.arity = arity;

    m_words[name] = std::move(def);
}

void Interpreter::executeIdentifier() {
    const Token& identTok = m_tokens[m_pos];
    const std::string name = std::get<std::string>(identTok.value);

    // variable cases...
    auto next = peek(1);
    if (next && next->type == TokenType::LOAD_VARIABLE) { /* ... */ }
    if (next && next->type == TokenType::STORE_VARIABLE) { /* ... */ }

    // user-defined word:
    auto it = m_words.find(name);
    if (it == m_words.end()) {
        throw std::runtime_error("[ERROR]: Unknown word '" + name + "'");
    }

    WordDef& def = it->second;

    if (def.arity > 0 && static_cast<int>(m_stack.size()) < def.arity) {
        throw std::runtime_error(
            "[ERROR]: Word '" + name + "' expects " +
            std::to_string(def.arity) + " argument(s) on the stack, but only " +
            std::to_string(m_stack.size()) + " present"
        );
    }

    // consume the identifier token
    ++m_pos;

    ControlSignal sig = executeBlock(def.body);

    if (sig == ControlSignal::Return) {
        // swallow return at word boundary
        return;
    }
    if (sig == ControlSignal::Break || sig == ControlSignal::Continue) {
        m_controlSignal = sig;
    }
}

void Interpreter::executeBreak() {
    consume(TokenType::BREAK, "[ERROR]: Expected 'break']");
    m_controlSignal = ControlSignal::Break;
}


void Interpreter::executeReturn() {
    consume(TokenType::RETURN, "[ERROR]: Expected 'return'");

    // We *don’t* know here whether we’re in a word or not.
    // We just signal RETURN and let callers decide if that is legal.
    m_controlSignal = ControlSignal::Return;
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


ControlSignal Interpreter::executeBlock(const std::vector<Token> &block) {
    const auto oldTokens = m_tokens;
    const auto oldPos = m_pos;
    const auto oldControlSignal = m_controlSignal;

    m_tokens = block;
    m_pos = 0;
    m_controlSignal = ControlSignal::None;

    auto result = ControlSignal::None;

    while (m_pos < m_tokens.size()) {
        if (const ControlSignal sig = executeSingleToken(); sig != ControlSignal::None) {
            result = sig;
            break;
        }
    }

    // Restore outer interpreter context
    m_tokens = oldTokens;
    m_pos = oldPos;
    m_controlSignal = oldControlSignal;

    return result;
}

bool Interpreter::isTruly(const StackValue &value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }

    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value) != 0; // Includes negative
    }

    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0.0; // Includes negative
    }

    if (std::holds_alternative<double>(value)) {
        return !std::get<std::string>(value).empty();
    }

    if (std::holds_alternative<std::string>(value)) {
        const auto& s = std::get<std::string>(value);
        return !s.empty();
    }

    return false;
}

std::vector<Token> Interpreter::collectUntil(const TokenType endType) {
    std::vector<Token> out;

    while (m_pos < m_tokens.size() && m_tokens[m_pos].type != endType) {
        out.push_back(consume());
    }

    if (m_pos >= m_tokens.size()) {
        throw std::runtime_error("[ERROR]: Unexpected end of input. Expected token: " + tokenTypeToString(endType));
    }

    return out;
}

std::pair<std::vector<Token>, std::vector<Token>> Interpreter::collectIfElseEndif(size_t ifIndex) {
    std::vector<Token> ifBranch;
    std::vector<Token> elseBranch;

    bool inElse = false;
    bool sawElse = false;
    int ifDepth = 1; // We are already inside an IF

    while (m_pos < m_tokens.size()) {
        const Token& tok = m_tokens[m_pos];

        switch (tok.type) {
            case TokenType::IF: {
                // Nested IF - increase depth, and store token in current branch
                ++ifDepth;
                (inElse ? elseBranch : ifBranch).push_back(consume());
                break;
            }
            case TokenType::ELSE: {
                if (ifDepth == 1) {
                    // ELSE for the current IF
                    if (sawElse) {
                        m_errorPos = m_pos;
                        throw std::runtime_error("[ERROR]: Multiple ELSE clauses in IF block");
                    }
                    sawElse = true;
                    inElse = true;
                    consume(TokenType::ELSE, "Expected ELSE after IF clause");
                    // DO NOT store the ELSE token itself
                } else {
                    // ELSE belongs to inner IF, treat as normal token
                    (inElse ? elseBranch : ifBranch).push_back(consume());
                }
                break;
            }
            case TokenType::ENDIF: {
                --ifDepth;
                if (ifDepth == 0) {
                    // This ENDIF closes the outer IF we are parsing
                    consume(TokenType::ENDIF, "[ERROR]: Expected ENDIF for IF block");
                    return {ifBranch, elseBranch};
                } else {
                    // ENDIF for inner IF, just store it
                    (inElse ? elseBranch : ifBranch).push_back(consume());
                }
                break;
            }
            default: {
                (inElse ? elseBranch : ifBranch).push_back(consume());
                break;
            }
        }
    }

    if (ifIndex < m_tokens.size()) {
        m_errorPos = ifIndex;
    } else {
        m_errorPos = static_cast<size_t>(-1); // Unknown, fallback
    }

    // If we reach this point, we ran out of tokens without closing the IF
    throw std::runtime_error("[ERROR]: Unbalanced IF: missing ENDIF before end of input");
}

std::vector<Token> Interpreter::collectBlockUntilEnd() {
    std::vector<Token> block;
    int depth = 1;

    while (m_pos < m_tokens.size()) {
        const Token& tok = m_tokens[m_pos];

        switch (tok.type) {
            // Any token that starts a nested block
            case TokenType::WHILE:
            case TokenType::WORD:
            case TokenType::IF: {
                ++depth;
                block.push_back(consume());
                break;
            }
            // Any token that ends a nested block
            case TokenType::END:
            case TokenType::ENDIF: {
                --depth;

                if (depth == 0) {
                    // This END/ENDIF closes the outer block we are collecting
                    consume(tok.type, "[ERROR]: Expected END/ENDIF to close block");
                    return block; // Do not include closing token
                }
                // It's an inner END/ENDIF, keep it in the body
                block.push_back(consume());
                break;
            }
            default: {
                block.push_back(consume());
                break;
            }
        }
    }

    // If we get here, we ran out of tokens before depth returned to 0
    throw std::runtime_error("[ERROR]: Unbalanced block: missing END before end of input");
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

    const StackValue v = m_stack.pop();

    const bool zeroLike = !isTruly(v);

    m_stack.push(zeroLike ? 1 : 0);
}

ControlSignal Interpreter::executeSingleToken() {
    if (m_pos >= m_tokens.size()) {
        return ControlSignal::None;
    }

    const Token& tok = m_tokens[m_pos];
    const auto type = tok.type;

    switch (tok.type) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STR_LITERAL:
        case TokenType::BOOL_LITERAL: {
            executePush(tok.value);
            consume();
            return ControlSignal::None;
        }
        case TokenType::IDENTIFIER: {
            m_controlSignal = ControlSignal::None;
            executeIdentifier();
            return m_controlSignal;
        }
        default: {
            // everything else via executionMap
            auto it = executionMap.find(type);
            if (it != executionMap.end()) {
                m_controlSignal = ControlSignal::None;
                it->second();            // handlers themselves call consume(TokenType::X, ...)
                return m_controlSignal;
            }
        }
    }

    throw std::runtime_error("[ERROR]: Unknown token type: " + tokenTypeToString(type));
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
        const Token& tok = m_tokens[m_pos];
        try {
           ControlSignal signal = executeSingleToken();

            if (signal == ControlSignal::Continue || signal == ControlSignal::Break || signal == ControlSignal::Return) {
                // At the top level, these are illegal
                std::string msg;
                if (signal == ControlSignal::Continue) msg = "[ERROR]: 'continue' used outside of the loop";
                else if (signal == ControlSignal::Break) msg = "[ERROR]: 'break' used outside of the loop]";
                else msg = "[ERROR]: 'return' used outside of a word";

                throw std::runtime_error(msg);
            }
        }
        catch (const std::runtime_error& e) {
            // Decide which position to report
            size_t reportPos = m_pos;

            if (m_errorPos != static_cast<size_t>(-1) &&
                m_errorPos < m_tokens.size()) {
                reportPos = m_errorPos;
                }

            std::cerr << "[ERROR]: Runtime error encountered!\n";
            std::cerr << "Message      : " << e.what() << "\n";
            std::cerr << "Current Pos  : " << reportPos << " / " << m_tokens.size() << "\n";

            if (reportPos < m_tokens.size()) {
                const Token& tok = m_tokens[reportPos];

                std::cerr << "Line         : " << tok.line << "\n";
                std::cerr << "Column       : " << tok.column << "\n";
                std::cerr << "Token Type   : " << tokenTypeToString(tok.type) << "\n";
                std::cerr << "Token Value  : ";
                if (std::holds_alternative<int>(tok.value)) {
                    std::cerr << std::get<int>(tok.value) << "\n";
                } else if (std::holds_alternative<double>(tok.value)) {
                    std::cerr << std::get<double>(tok.value) << "\n";
                } else if (std::holds_alternative<std::string>(tok.value)) {
                    std::cerr << std::get<std::string>(tok.value) << "\n";
                } else {
                    std::cerr << "<none>\n";
                }

                printRuntimeErrorContext(tok.line, tok.column);
            } else {
                std::cerr << "Token        : <Position beyond token list>\n";
            }

            // Reset error pos so the next error isn’t polluted
            m_errorPos = static_cast<size_t>(-1);

            std::cerr << "Execution halted.\n";
            std::exit(EXIT_FAILURE);
        }
    }
}



