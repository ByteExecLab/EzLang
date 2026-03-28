#include "Interpreter.h"

#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <variant>
#include <type_traits>
#include <iomanip> // for std::setw, std::left

#include "EzError.h"
#include "Utils.h"

namespace {
    EzEnvironmentPtr findEnvironmentWithBinding(const EzEnvironmentPtr& env, const std::string& name) {
        for (auto current = env; current; current = current->parent) {
            if (!current->table) {
                continue;
            }

            if (current->table->entries.contains(EzTableKey{name})) {
                return current;
            }
        }

        return nullptr;
    }

    std::shared_ptr<StackValue> findEnvironmentBinding(const EzEnvironmentPtr& env, const std::string& name) {
        if (const auto owner = findEnvironmentWithBinding(env, name)) {
            return owner->table->entries.at(EzTableKey{name});
        }

        return {};
    }

    const EzTable& expectTable(const StackValue& value, const char* message) {
        if (!std::holds_alternative<EzTablePtr>(value)) {
            throw std::runtime_error(message);
        }

        const auto& table = std::get<EzTablePtr>(value);
        if (!table) {
            throw std::runtime_error(message);
        }

        return *table;
    }

    EzTable& expectTable(StackValue& value, const char* message) {
        if (!std::holds_alternative<EzTablePtr>(value)) {
            throw std::runtime_error(message);
        }

        const auto& table = std::get<EzTablePtr>(value);
        if (!table) {
            throw std::runtime_error(message);
        }

        return *table;
    }
}

Interpreter::Interpreter(std::vector<Token> tokens,
                         Stack stack,
                         std::string source,
                         std::string moduleName,
                         EzEnvironmentPtr globalEnv)
    : m_source(std::move(source)),
      m_stack(std::move(stack)),
      m_tokens(std::move(tokens)),
      m_moduleName(std::move(moduleName)),
      m_globalEnv(std::move(globalEnv)) {

    if (!m_globalEnv) {
        m_globalEnv = std::make_shared<EzEnvironment>();
    }
    if (!m_globalEnv->table) {
        m_globalEnv->table = std::make_shared<EzTable>();
    }

    // Global frame
    m_frames.emplace_back();

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
        // {TokenType::CONST, [this]() { executeDefineVariable(); }},
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
        {TokenType::WORD, [this]() { executeWordDefinition(); }},

        // Arrays & Structs
        {TokenType::ARRAY_START,  [this]() { executeArrayStart(); }},
        {TokenType::ARRAY_END,    [this]() { executeArrayEnd(); }},
        {TokenType::STRUCT_START, [this]() { executeStructStart(); }},
        {TokenType::STRUCT_END,   [this]() { executeStructEnd(); }},

        {TokenType::ARRAY_LEN,     [this]() { executeArrayLen(); }},
        {TokenType::ARRAY_GET,     [this]() { executeArrayGet(); }},
        {TokenType::ARRAY_SET,     [this]() { executeArraySet(); }},
        {TokenType::STRUCT_GET,    [this]() { executeStructGet(); }},
        {TokenType::STRUCT_SET,    [this]() { executeStructSet(); }},
        {TokenType::STRUCT_ACCESS, [this]() { executeStructAccess(); }},
    };
}

void Interpreter::printVariant(const StackValue& value) {
    std::visit(Utils::PrintVisitor{}, value);
    std::cout << "";
}

Token Interpreter::getCurrentToken() const {
    return m_tokens.at(m_pos);
}


template<typename T>
bool Interpreter::isOfType(StackValue &value) {
    return std::holds_alternative<T>(value);
}

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

bool Interpreter::toBool(const StackValue &v) {
    return isTruly(v);
}

double Interpreter::toDouble(const StackValue &v) {
    if (std::holds_alternative<int>(v)) {
        return std::get<int>(v);
    }

    if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    }

    if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? 1.0 : 0.0;
    }

    throw std::runtime_error("[ERROR]: Expected numeric value, got non-numeric type");
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

                if (const BlockKind kind = stack.back().kind; kind == BlockKind::While || kind == BlockKind::Word) {
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

// TODO: Move this to stack class
void Interpreter::ensureStackSize(const size_t needed, const std::string &opName) const {
     if (m_stack.size() < needed) {
         throw std::runtime_error(
             "[ERROR]: Stack underflow for '" + opName +
             "' — requires " + std::to_string(needed) +
             " values, but only " + std::to_string(m_stack.size()) + " present"
         );
     }
}

std::string Interpreter::GetStringOrThrow(const StackValue &value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    throw std::runtime_error("[ERROR]: Expected string value on stack");
}

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

std::optional<Token> Interpreter::peek(const size_t offset) {
    if (m_pos + offset >= m_tokens.size()) {
        return std::nullopt;
    }

    return m_tokens.at(m_pos + offset);
}

void Interpreter::executePrint() {
    consume(TokenType::PRINT, "[ERROR]: Expected PRINT");
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack underflow for print operation");
    }

    const StackValue v = m_stack.pop();   // or peek() if you like
    printVariant(v);
    std::cout << std::endl;
}

void Interpreter::executePush(const StackValue &value) {
    m_stack.push(value);
}

void Interpreter::executeStrLiteral() {
    const auto [type, value, line, col] =
        consume(TokenType::STR_LITERAL, "[ERROR]: Expected string literal");
    m_stack.push(value);
}

void Interpreter::executeIntLiteral() {
    const auto [type, value, line, col] = consume(TokenType::INT_LITERAL, "[ERROR]: Expected integer literal");
    m_stack.push(value);
}

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

void Interpreter::executeBinary(const TokenType tokenType) {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for binary operation");
    }

    // Stack: [..., left, right]
    const StackValue r_value = m_stack.pop(); // TOP
    const StackValue l_value = m_stack.pop(); // Below TOP

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
                    const int a = std::get<int>(l_value);
                    const int b = std::get<int>(r_value);
                    result = a + b;
                }
                else {
                    const double a = toDouble(l_value);
                    const double b = toDouble(r_value);
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
                const int a = std::get<int>(l_value);
                const int b = std::get<int>(r_value);

                // Left - Right
                result = a - b;
            } else {
                const double a = toDouble(l_value);
                const double b = toDouble(r_value);
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
                const int a = std::get<int>(l_value);
                const int b = std::get<int>(r_value);

                result = a * b;
            } else {
                const double a = toDouble(l_value);
                const double b = toDouble(r_value);

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

            const double a = toDouble(l_value);
            const double b = toDouble(r_value);

            if (b == 0.0) {
                throw std::runtime_error("[ERROR]: Division by zero");
            }

            // Int / Int = Int, else double
            if (bothInt(l_value, r_value)) {
                const int ia = std::get<int>(l_value);
                const int ib = std::get<int>(r_value);

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

            const int a = std::get<int>(l_value);
            const int b = std::get<int>(r_value);

            if (b == 0) {
                throw std::runtime_error("[ERROR]: Modulo by zero");
            }

            result = a % b; // left % right
            break;
        }
        default: {
            //
        }
    }

    m_stack.push(result);
}

void Interpreter::executeLogical(const TokenType op) {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack underflow for logical operation");
    }

    const StackValue r_value = m_stack.pop(); //top
    const StackValue l_value = m_stack.pop(); // below top

    // Helper lambda for numeric comparison
    auto toDouble = [&](const StackValue& v) -> double {
        return std::visit([]<typename T0>(T0&& val) -> double {
            using T = std::decay_t<T0>;

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

void Interpreter::executeWhile() {
    consume(TokenType::WHILE, "[ERROR]: Expected WHILE operation");

    const std::vector<Token> conditionTokens = collectUntil(TokenType::DO);
    consume(TokenType::DO, "[ERROR]: Expected DO after WHILE condition");

    const std::vector<Token> bodyTokens = collectBlockUntilEnd();
    // consume(TokenType::END, "[ERROR]: Expected END after WHILE block");

    // Save outer signal so we don't leak inner loop control outwards
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

// TODO: Might need to be removed
void Interpreter::executeDefineVariable() {
    //
}

void Interpreter::defineVariable(const bool isConst) {
    // Stack: [..., initialValue]
    if (m_stack.empty()) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Cannot define a variable: stack is empty");
    }

    // The next token must be identifier
    const Token nameTok = consume(TokenType::IDENTIFIER, "[INTERPRETER][ERROR]: Expected identifier after const/var");
    const auto& name = std::get<std::string>(nameTok.value);

    if (m_globalEnv->table->entries.contains(EzTableKey{name})) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Variable '" + name + "' has already been defined");
    }

    const StackValue initial = m_stack.pop();
    m_globalEnv->table->entries[EzTableKey{name}] = std::make_shared<StackValue>(initial);

    if (isConst) {
        m_globalEnv->constNames.insert(name);
    } else {
        m_globalEnv->constNames.erase(name);
    }
}

void Interpreter::executeDefineConst() {
    consume(TokenType::CONST, "[INTERPRETER][ERROR]: expected const");
    defineVariable(true);
}

void Interpreter::executeDefineVar() {
    consume(TokenType::VAR, "[INTERPRETER][ERROR]: expected var");
    defineVariable(false);
}

void Interpreter::registerNativeWord(const std::string &name, const int arity, std::function<ControlSignal(Interpreter &)> fn) {
    m_nativeWords[name] = NativeWord{arity, std::move(fn)};
}

void Interpreter::registerHostFunction(const std::string& name, const int arity, EzHostFunction fn) {
    registerNativeWord(name, arity, [arity, fn = std::move(fn)](Interpreter& interpreter) -> ControlSignal {
        auto& st = interpreter.stack();
        if (arity > 0 && static_cast<int>(st.size()) < arity) {
            throw std::runtime_error(
                "[ERROR]: Host function expects " + std::to_string(arity) +
                " argument(s), but only " + std::to_string(st.size()) + " present"
            );
        }

        std::vector<StackValue> args;
        args.reserve(arity);
        for (int i = 0; i < arity; ++i) {
            args.push_back(st.pop());
        }

        std::reverse(args.begin(), args.end());

        const auto outputs = fn(args);
        for (const auto& value : outputs) {
            st.push(value);
        }

        return ControlSignal::None;
    });
}

Stack &Interpreter::stack() {
    return m_stack;
}

void Interpreter::setRuntimeLimits(const size_t instructionBudget,
                                   const size_t maxCallDepth,
                                   std::function<bool()> cancelRequested) {
    m_instructionBudget = instructionBudget;
    m_maxCallDepth = maxCallDepth;
    m_cancelRequested = std::move(cancelRequested);
}

void Interpreter::resetRuntimeCounters() {
    m_instructionCount = 0;
    m_callDepth = 0;
}

void Interpreter::pushUserData(std::shared_ptr<void> handle, std::string typeName) {
    m_stack.push(std::make_shared<EzUserData>(EzUserData{
        .handle = std::move(handle),
        .typeName = std::move(typeName)
    }));
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
    std::vector<Token> body = collectBlockUntilEnd();

    WordDef def;
    def.body  = std::move(body);
    def.arity = arity;

    m_words[name] = std::move(def);
}

void Interpreter::executeIdentifier() {
    const Token& identToken = m_tokens[m_pos];
    const std::string name = std::get<std::string>(identToken.value);

    // Bare identifiers first resolve against the current local frame.
    // This keeps locals frame-scoped and prevents implicit reads from globals.
    auto& locals = currentFrame().locals;
    if (const auto itLocal = locals.find(name); itLocal != locals.end()) {
        ++m_pos; // consume IDENTIFIER
        m_stack.push(itLocal->second);
        return;
    }

    if (const auto itUser = m_words.find(name); itUser != m_words.end()) {
        ++m_pos; // consume IDENTIFIER
        const ControlSignal sig = invokeUserWord(name, itUser->second);

        if (sig == ControlSignal::Break || sig == ControlSignal::Continue) {
            m_controlSignal = sig;
        }
        return;
    }

    const auto next = peek(1);
    if (next && next->type == TokenType::LOAD_VARIABLE) {
        executeLoadVariable();
        return;
    }
    if (next && next->type == TokenType::STORE_VARIABLE) {
        executeStoreVariable();
        return;
    }

    if (const auto itNative = m_nativeWords.find(name); itNative != m_nativeWords.end()) {
        ++m_pos; // consume IDENTIFIER
        const ControlSignal sig = invokeNativeWord(name, itNative->second);

        if (sig == ControlSignal::Break || sig == ControlSignal::Continue) {
            m_controlSignal = sig;
        }
        return;
    }

    throw std::runtime_error("[ERROR]: Unknown word '" + name + "'");
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

void Interpreter::executeLoadVariable() {
    const Token nameTok = consume(TokenType::LOAD_VARIABLE,
        "[INTERPRETER][ERROR]: Expected '@' before identifier");

    const auto& name = std::get<std::string>(nameTok.value);
    m_stack.push(getGlobal(name));
}

void Interpreter::executeAnd() {
    consume(TokenType::AND, "Expected 'and'");
    ensureStackSize(2, "AND");

    const StackValue b = m_stack.pop();
    const StackValue a = m_stack.pop();

    const bool result = isTruly(a) && isTruly(b);
    m_stack.push(static_cast<int>(result));
}

void Interpreter::executeOr() {
    consume(TokenType::OR, "Expected 'or'");
    ensureStackSize(2, "OR");

    const StackValue b = m_stack.pop();
    const StackValue a = m_stack.pop();

    const bool result = isTruly(a) || isTruly(b);
    m_stack.push(static_cast<int>(result));
}

void Interpreter::executeNot() {
    consume(TokenType::NOT, "Expected 'not'");
    ensureStackSize(1, "NOT");

    const StackValue a = m_stack.pop();

    const bool result = !isTruly(a);
    m_stack.push(static_cast<int>(result));
}

void Interpreter::executeArrayStart() {
    consume(TokenType::ARRAY_START, "Expected '['");
    m_arrayMarks.push_back(m_stack.size());
}

void Interpreter::executeArrayEnd() {
    consume(TokenType::ARRAY_END, "Expected ']'");

    if (m_arrayMarks.empty()) {
        throw std::runtime_error("[ERROR]: Unmatched ']' with no '['");
    }

    const size_t startSize = m_arrayMarks.back();
    m_arrayMarks.pop_back();

    if (m_stack.size() < startSize) {
        throw std::runtime_error("[ERROR]: Internal: stack smaller than array start mark");
    }

    const size_t count = m_stack.size() - startSize;
    std::vector<StackValue> elements;
    elements.reserve(count);

    // Pop values above the mark
    for (size_t i = 0; i < count; ++i) {
        elements.push_back(m_stack.pop());
    }

    // Reverse to restore left-to-right literal order
    std::reverse(elements.begin(), elements.end());

    m_stack.push(std::make_shared<EzTable>(std::move(elements)));
}

void Interpreter::executeStructStart() {
    consume(TokenType::STRUCT_START, "Expected '{'");
    m_structMarks.push_back(m_stack.size());
}

bool Interpreter::hasWord(const std::string &name) const {
    return m_words.contains(name) || m_nativeWords.contains(name);
}

std::vector<StackValue> Interpreter::callWord(const std::string &name, const std::vector<StackValue> &args) {
    const size_t stackBase = m_stack.size();

    for (const auto& arg : args) {
        m_stack.push(arg);
    }

    try {
        ControlSignal sig = ControlSignal::None;

        if (const auto itUser = m_words.find(name); itUser != m_words.end()) {
            sig = invokeUserWord(name, itUser->second);
        } else if (const auto itNative = m_nativeWords.find(name); itNative != m_nativeWords.end()) {
            sig = invokeNativeWord(name, itNative->second);
        } else {
            throw std::runtime_error("[ERROR]: Unknown word '" + name + "'");
        }

        if (sig == ControlSignal::Break) {
            throw std::runtime_error("[ERROR]: 'break' propagated out of word '" + name + "'");
        }
        if (sig == ControlSignal::Continue) {
            throw std::runtime_error("[ERROR]: 'continue' propagated out of word '" + name + "'");
        }

        std::vector<StackValue> outputs;
        while (m_stack.size() > stackBase) {
            outputs.push_back(m_stack.pop());
        }

        std::reverse(outputs.begin(), outputs.end());
        return outputs;
    } catch (const EzException&) {
        while (m_stack.size() > stackBase) {
            m_stack.pop();
        }
        throw;
    } catch (const std::runtime_error& e) {
        while (m_stack.size() > stackBase) {
            m_stack.pop();
        }
        throw makeRuntimeException(e.what());
    }
}

void Interpreter::executeStructEnd() {
    consume(TokenType::STRUCT_END, "Expected '}'");

    if (m_structMarks.empty()) {
        throw std::runtime_error("[ERROR]: Unmatched '}' with no '{'");
    }

    const size_t startSize = m_structMarks.back();
    m_structMarks.pop_back();

    if (m_stack.size() < startSize) {
        throw std::runtime_error("[ERROR]: Internal: stack smaller than struct start mark");
    }

    const size_t count = m_stack.size() - startSize;

    if (count % 2 != 0) {
        throw std::runtime_error("[ERROR]: Struct literal expects key/value pairs (even number of stack items)");
    }

    EzTable obj;

    // We pop value then key, in reverse of push order.
    for (size_t i = 0; i < count / 2; ++i) {
        StackValue value = m_stack.pop();
        StackValue key   = m_stack.pop();

        if (!std::holds_alternative<std::string>(key)) {
            throw std::runtime_error("[ERROR]: Struct keys must be strings");
        }

        auto keyStr = std::get<std::string>(key);
        obj.entries.emplace(EzTableKey{std::move(keyStr)}, std::make_shared<StackValue>(std::move(value)));
    }

    // Order doesn't matter because it's a map.
    m_stack.push(std::make_shared<EzTable>(std::move(obj)));
}

void Interpreter::executeArrayLen() {
    consume(TokenType::ARRAY_LEN, "[ERROR]: Expected array-len");

    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: array-len: stack underflow");
    }

    StackValue v = m_stack.pop();

    const auto& arr = expectTable(v, "[ERROR]: array-len: expected table on stack");
    const int len = static_cast<int>(arr.arrayEntryCount());

    // push length back
    m_stack.push(len);
}

void Interpreter::executeArrayGet() {
    consume(TokenType::ARRAY_GET, "Expected 'array-get'");

    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: array-get requires array and index");
    }

    const StackValue idxV  = m_stack.pop();
    const StackValue arrV  = m_stack.pop();

    const int idx = GetIntOrThrow(idxV);

    const auto& arr = expectTable(arrV, "[ERROR]: array-get expects table under index");
    const auto it = arr.entries.find(EzTableKey{idx});
    if (idx < 0 || it == arr.entries.end()) {
        throw std::runtime_error("[ERROR]: array-get index out of range");
    }

    m_stack.push(*it->second);
}


 void Interpreter::executeArraySet() {
    consume(TokenType::ARRAY_SET, "Expected 'array-set'");

    if (m_stack.size() < 3) {
        throw std::runtime_error("[ERROR]: array-set requires array, index, value");
    }

    StackValue valueV = m_stack.pop();
    const StackValue idxV   = m_stack.pop();
    StackValue arrV   = m_stack.pop();

    const int idx = GetIntOrThrow(idxV);

    auto& arr = expectTable(arrV, "[ERROR]: array-set expects table under index/value");
    const auto it = arr.entries.find(EzTableKey{idx});
    if (idx < 0 || it == arr.entries.end()) {
        throw std::runtime_error("[ERROR]: array-set index out of range");
    }

    arr.entries[EzTableKey{idx}] = std::make_shared<StackValue>(std::move(valueV));

    m_stack.push(arrV);
 }

void Interpreter::executeStructGet() {
    consume(TokenType::STRUCT_GET, "Expected 'struct-get'");

    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: struct-get requires struct and key");
    }

    StackValue keyV = m_stack.pop();
    StackValue objV = m_stack.pop();

    if (!std::holds_alternative<std::string>(keyV)) {
        throw std::runtime_error("[ERROR]: struct-get expects string key");
    }
    const std::string& key = std::get<std::string>(keyV);
    const auto& obj = expectTable(objV, "[ERROR]: struct-get expects table under key");

    const auto it = obj.entries.find(EzTableKey{key});
    if (it == obj.entries.end()) {
        m_stack.push(StackValue{std::monostate{}});
    } else {
        m_stack.push(*it->second);
    }
}

void Interpreter::executeStructSet() {
    consume(TokenType::STRUCT_SET, "Expected 'struct-set'");

    if (m_stack.size() < 3) {
        throw std::runtime_error("[ERROR]: struct-set requires struct, key, value");
    }

    StackValue valueV = m_stack.pop();
    StackValue keyV   = m_stack.pop();
    StackValue objV   = m_stack.pop();

    if (!std::holds_alternative<std::string>(keyV)) {
        throw std::runtime_error("[ERROR]: struct-set expects string key");
    }
    auto key = std::get<std::string>(keyV);
    auto& obj = expectTable(objV, "[ERROR]: struct-set expects table under key/value");

    obj.entries[EzTableKey{std::move(key)}] = std::make_shared<StackValue>(std::move(valueV));

    m_stack.push(objV);
}

void Interpreter::executeStructAccess() {
    consume(TokenType::STRUCT_ACCESS, "Expected '.'");

    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: '.' requires struct and key on stack");
    }

    StackValue keyV = m_stack.pop();
    StackValue objV = m_stack.pop();

    if (!std::holds_alternative<std::string>(keyV)) {
        throw std::runtime_error("[ERROR]: '.' expects string key on top");
    }
    const std::string& key = std::get<std::string>(keyV);
    const auto& obj = expectTable(objV, "[ERROR]: '.' expects table under key");

    const auto it = obj.entries.find(EzTableKey{key});
    if (it == obj.entries.end()) {
        m_stack.push(StackValue{std::monostate{}});
    } else {
        m_stack.push(*it->second);
    }
}

void Interpreter::executeStoreVariable() {
    if (m_stack.empty()) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Cannot store variable: Stack is empty");
    }

    // Consume '!' (STORE_VARIABLE)
    consume();

    const Token nameTok = consume(TokenType::IDENTIFIER, "[INTERPRETER][ERROR]: Expected identifier before '!'");
    const auto& name = std::get<std::string>(nameTok.value);

    const auto envWithBinding = findEnvironmentWithBinding(m_globalEnv, name);
    if (!envWithBinding) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Undefined variable '" + name + "'");
    }

    if (envWithBinding->constNames.contains(name)) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Cannot assign to const '" + name + "'");
    }

    const StackValue value = m_stack.pop();
    envWithBinding->table->entries[EzTableKey{name}] = std::make_shared<StackValue>(value);
}

void Interpreter::executeLet() {
    consume(TokenType::LET, "[INTERPRETER][ERROR]: Expected 'let'");

    const Token nameTok = consume(TokenType::IDENTIFIER, "[INTERPRETER][ERROR]: Expected identifier name after 'let'");
    const std::string name = std::get<std::string>(nameTok.value);

    if (m_stack.empty()) {
        throw std::runtime_error("[INTERPRETER][ERROR]: 'let " + name + "': Stack underflow");
    }

    const StackValue value = m_stack.pop();
    currentFrame().locals[name] = value;
}

void Interpreter::executeSet() {
    consume(TokenType::SET, "[INTERPRETER][ERROR]: Expected 'set'");

    const Token nameTok = consume(TokenType::IDENTIFIER, "[INTERPRETER][ERROR]: Expected identifier name after 'set'");
    const std::string name = std::get<std::string>(nameTok.value);

    auto &locals = currentFrame().locals;
    const auto it  = locals.find(name);
    if (it == locals.end()) {
        throw std::runtime_error("[INTERPRETER][ERROR]: 'set " + name + "': local not defined in this frame");
    }

    if (m_stack.empty()) {
        throw std::runtime_error("[INTERPRETER][ERROR]: 'set " + name + "': Stack underflow");
    }

    const StackValue value = m_stack.pop();
    it->second = value;
}

ControlSignal Interpreter::executeBlock(const std::vector<Token> &block) {
    const auto oldTokens = m_tokens;
    const auto oldPos = m_pos;
    const auto oldControlSignal = m_controlSignal;

    m_tokens = block;
    m_pos = 0;
    m_controlSignal = ControlSignal::None;

    try {
        auto result = ControlSignal::None;

        while (m_pos < m_tokens.size()) {
            if (const ControlSignal sig = executeSingleToken(); sig != ControlSignal::None) {
                result = sig;
                break;
            }
        }

        m_tokens = oldTokens;
        m_pos = oldPos;
        m_controlSignal = oldControlSignal;
        return result;
    } catch (const EzException&) {
        m_tokens = oldTokens;
        m_pos = oldPos;
        m_controlSignal = oldControlSignal;
        throw;
    } catch (const std::runtime_error& e) {
        const auto wrapped = makeRuntimeException(e.what());
        m_tokens = oldTokens;
        m_pos = oldPos;
        m_controlSignal = oldControlSignal;
        throw wrapped;
    }
}

bool Interpreter::isTruly(const StackValue &value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return false; // Nil is always false
    }

    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }

    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value) != 0; // Includes negative
    }

    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0.0; // Includes negative
    }

    if (std::holds_alternative<std::string>(value)) {
        const auto& s = std::get<std::string>(value);
        return !s.empty();
    }

    if (std::holds_alternative<EzTablePtr>(value)) {
        return true;
    }

    if (std::holds_alternative<EzUserDataPtr>(value)) {
        return true;
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

Frame &Interpreter::currentFrame() {
    if (m_frames.empty()) {
        throw std::runtime_error("[ERROR]: No active frame");
    }

    return m_frames.back();
}

void Interpreter::pushFrame() {
    m_frames.emplace_back();
}

void Interpreter::popFrame() {
    if (m_frames.empty()) {
        throw std::runtime_error("[ERROR]: Frame stack underflow");
    }

    m_frames.pop_back();
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

ControlSignal Interpreter::invokeUserWord(const std::string& name, const WordDef& def) {
    if (def.arity > 0 && static_cast<int>(m_stack.size()) < def.arity) {
        throw std::runtime_error(
            "[ERROR]: Word '" + name + "' expects " +
            std::to_string(def.arity) + " argument(s) on the stack, but only " +
            std::to_string(m_stack.size()) + " present"
        );
    }

    if (m_callDepth >= m_maxCallDepth) {
        throw makeRuntimeException("[ERROR]: Maximum call depth exceeded");
    }

    pushFrame();
    ++m_callDepth;

    try {
        const ControlSignal sig = executeBlock(def.body);
        --m_callDepth;
        popFrame();

        if (sig == ControlSignal::Return) {
            return ControlSignal::None;
        }

        return sig;
    } catch (...) {
        --m_callDepth;
        popFrame();
        throw;
    }
}

ControlSignal Interpreter::invokeNativeWord(const std::string& name, NativeWord& def) {
    if (def.arity > 0 && static_cast<int>(m_stack.size()) < def.arity) {
        throw std::runtime_error(
            "[ERROR]: Native word '" + name + "' expects " +
            std::to_string(def.arity) + " argument(s) on the stack, but only " +
            std::to_string(m_stack.size()) + " present"
        );
    }

    const ControlSignal sig = def.fn(*this);

    if (sig == ControlSignal::Return) {
        return ControlSignal::None;
    }

    return sig;
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

    if (++m_instructionCount > m_instructionBudget) {
        throw makeRuntimeException("[ERROR]: Instruction budget exceeded");
    }

    if (m_cancelRequested && m_cancelRequested()) {
        throw makeRuntimeException("[ERROR]: Execution cancelled by host");
    }

    const Token& tok = m_tokens[m_pos];
    const auto type = tok.type;

    switch (tok.type) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STR_LITERAL:
        case TokenType::BOOL_LITERAL:
        case TokenType::NIL_LITERAL: {
            executePush(tok.value);
            consume();
            return ControlSignal::None;
        }
        case TokenType::IDENTIFIER: {
            m_controlSignal = ControlSignal::None;
            executeIdentifier();
            return m_controlSignal;
        }

        // Variables
        case TokenType::CONST: {
            executeDefineConst();
            return ControlSignal::None;
        }
        case TokenType::VAR: {
            executeDefineVar();
            return ControlSignal::None;
        }
        case TokenType::LOAD_VARIABLE: {
            executeLoadVariable();
            return ControlSignal::None;
        }
        case TokenType::STORE_VARIABLE: {
            executeStoreVariable();
            return ControlSignal::None;
        }
        // logical ops:
        case TokenType::AND: {
            executeAnd();
            return ControlSignal::None;
        }
        case TokenType::OR: {
            executeOr();
            return ControlSignal::None;
        }
        case TokenType::NOT: {
            executeNot();
            return ControlSignal::None;
        }
        // Locals
        case TokenType::LET: {
            executeLet();
            return ControlSignal::None;
        }
        case TokenType::SET: {
            executeSet();
            return ControlSignal::None;
        }
        default: {
            // everything else via executionMap
            if (const auto it = executionMap.find(type); it != executionMap.end()) {
                m_controlSignal = ControlSignal::None;
                it->second();            // handlers themselves call consume(TokenType::X, ...)
                return m_controlSignal;
            }
        }
    }

    throw std::runtime_error("[ERROR]: Unknown token type: " + tokenTypeToString(type));
}

bool Interpreter::hasGlobal(const std::string &name) const {
    return static_cast<bool>(findEnvironmentBinding(m_globalEnv, name));
}

StackValue Interpreter::getGlobal(const std::string &name) const {
    const auto binding = findEnvironmentBinding(m_globalEnv, name);
    if (!binding) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Undefined variable '" + name + "'");
    }

    return *binding;
}

void Interpreter::setGlobal(const std::string &name, const StackValue &value, bool isConst) {
    auto& entries = m_globalEnv->table->entries;
    const auto it = entries.find(EzTableKey{name});

    if (it != entries.end()) {
        if (m_globalEnv->constNames.contains(name)) {
            throw std::runtime_error("[INTERPRETER][ERROR]: Cannot assign to const '" + name + "'");
        }

        entries[EzTableKey{name}] = std::make_shared<StackValue>(value);
        return;
    }

    entries[EzTableKey{name}] = std::make_shared<StackValue>(value);
    if (isConst) {
        m_globalEnv->constNames.insert(name);
    }
}

void Interpreter::setGlobalEnvironment(EzEnvironmentPtr env) {
    if (!env) {
        throw std::runtime_error("[INTERPRETER][ERROR]: Global environment cannot be null");
    }
    if (!env->table) {
        env->table = std::make_shared<EzTable>();
    }

    m_globalEnv = std::move(env);
}

EzEnvironmentPtr Interpreter::globalEnvironment() const {
    return m_globalEnv;
}

void Interpreter::execute() {
    while (m_pos < m_tokens.size()) {
        try {
            if (const ControlSignal signal = executeSingleToken(); signal == ControlSignal::Continue || signal == ControlSignal::Break || signal == ControlSignal::Return) {
                // At the top level, these are illegal
                std::string msg;
                if (signal == ControlSignal::Continue) msg = "[ERROR]: 'continue' used outside of the loop";
                else if (signal == ControlSignal::Break) msg = "[ERROR]: 'break' used outside of the loop]";
                else msg = "[ERROR]: 'return' used outside of a word";

                throw std::runtime_error(msg);
            }
        }
        catch (const EzException&) {
            throw;
        }
        catch (const std::runtime_error& e) {
            throw makeRuntimeException(e.what());
        }
    }
}

std::string Interpreter::buildRuntimeErrorContext(const size_t line, const size_t column) const {
    size_t idx = 0;
    size_t currentLine = 1;

    while (currentLine < line && idx < m_source.size()) {
        if (m_source[idx] == '\n') {
            currentLine++;
        }
        idx++;
    }

    const size_t lineStart = idx;
    while (idx < m_source.size() && m_source[idx] != '\n') {
        idx++;
    }
    const size_t lineEnd = idx;

    const std::string lineStr = m_source.substr(lineStart, lineEnd - lineStart);

    std::ostringstream out;
    out << "    " << lineStr << "\n";
    out << "    ";
    for (size_t i = 1; i < column; ++i) {
        out << ' ';
    }
    out << "^\n";
    return out.str();
}

EzException Interpreter::makeRuntimeException(const std::string& message) {
    size_t reportPos = m_pos;

    if (m_errorPos != static_cast<size_t>(-1) && m_errorPos < m_tokens.size()) {
        reportPos = m_errorPos;
    }

    EzSourceLocation location{m_moduleName, 0, 0};
    std::string snippet;

    if (reportPos < m_tokens.size()) {
        const Token& token = m_tokens[reportPos];
        location.line = token.line;
        location.column = token.column;
        snippet = buildRuntimeErrorContext(token.line, token.column);
    }

    m_errorPos = static_cast<size_t>(-1);

    return EzException(EzError{
        .phase = EzErrorPhase::Runtime,
        .message = message,
        .location = std::move(location),
        .snippet = std::move(snippet)
    });
}

void Interpreter::printRuntimeErrorContext(const size_t line, const size_t column) const {
    std::cerr << buildRuntimeErrorContext(line, column);
}



