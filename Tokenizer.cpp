#include "Tokenizer.h"

#include <iostream>
#include <unordered_map>

#include "EzError.h"

Tokenizer::Tokenizer(std::string source, std::string moduleName): m_source(std::move(source)), m_moduleName(std::move(moduleName)) {}

auto Tokenizer::readNumber(std::string prefix = "") {
    std::string num = std::move(prefix);
    bool isFloat = false;

    // Digits before decimal
    while (peek().has_value() && std::isdigit(peek().value())) {
        num += consume();
    }

    // Decimal part
    if (peek().has_value() && peek().value() == '.') {
        isFloat = true;
        num += consume();
        while (peek().has_value() && std::isdigit(peek().value())) {
            num += consume();
        }
    }

    try {
        if (isFloat) {
            m_tokens.emplace_back(TokenType::FLOAT_LITERAL, std::stod(num));
        } else {
            m_tokens.emplace_back(TokenType::INT_LITERAL, std::stoi(num));
        }
    } catch (const std::exception& e) {
        throwTokenizeError("[LEXER][ERROR]: Number parsing error: " +  std::string(e.what()), m_line, m_column);
    }
}

std::vector<Token> Tokenizer::tokenize() {
    std::string buf;

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"dup", TokenType::DUP},
        {"swap", TokenType::SWAP},
        {"over", TokenType::OVER},
        {"nip", TokenType::NIP},
        {"tuck", TokenType::TUCK},
        {"drop", TokenType::DROP},
        {"print", TokenType::PRINT},
        {"if", TokenType::IF},
        {"and", TokenType::AND},
        {"or", TokenType::OR},
        {"not", TokenType::NOT},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"do", TokenType::DO},
        {"end", TokenType::END},
        {"trace", TokenType::TRACE},
        {"endif", TokenType::ENDIF},
        {"continue", TokenType::CONTINUE},
        {"break", TokenType::BREAK},
        {"return", TokenType::RETURN},
        {"word", TokenType::WORD},


        // Variables
        {"const", TokenType::CONST},
        {"var", TokenType::VAR},

        // Locals
        {"let", TokenType::LET},
        {"set", TokenType::SET},

        // Arrays & Structs
        {"array-len",  TokenType::ARRAY_LEN},
        {"array-get",  TokenType::ARRAY_GET},
        {"array-set",  TokenType::ARRAY_SET},
        {"struct-get", TokenType::STRUCT_GET},
        {"struct-set", TokenType::STRUCT_SET},
    };

    while (peek().has_value()) {
        switch (const char c = consume()) {
            // --- Whitespace / comments ---
            case ' ':
            case '\t': {
                break;
            }
            case '\n':
            case '\r': {
                // newline: we consider this a "statement boundary", so allow unary - after line break
                break;
            }
            case ';': {
                // Skip comment until end of line
                while (peek().has_value() && peek().value() != '\n') consume();
                break;
            }
            case '[': {
                m_tokens.emplace_back(TokenType::ARRAY_START, std::string{}, m_line, m_column);
                break;
            }
            case ']': {
                m_tokens.emplace_back(TokenType::ARRAY_END, std::string{}, m_line, m_column);
                // acts like an operator, not a literal → keep lastWasValue = false
                break;
            }
            case '{': {
                m_tokens.emplace_back(TokenType::STRUCT_START, std::string{}, m_line, m_column);
                break;
            }
            case '}': {
                m_tokens.emplace_back(TokenType::STRUCT_END, std::string{}, m_line, m_column);
                break;
            }
            // --- Operators / punctuation ---
            case '+': {
                m_tokens.emplace_back(TokenType::ADD, '+', m_line, m_column);
                break;
            }
            case '.': {
                m_tokens.emplace_back(TokenType::STRUCT_ACCESS, std::string{}, m_line, m_column);
                break;
            }
            case '-': {
                auto next = peek();
                const bool nextIsDigit = next.has_value() &&
                                   std::isdigit(static_cast<unsigned char>(next.value()));

                if (nextIsDigit) {
                    // We've already consumed '-', so pass "-" as prefix
                    readNumber("-");
                } else {
                    // Binary subtraction operator
                    m_tokens.emplace_back(TokenType::SUB, std::string{}, m_line, m_column);
                }
                break;
            }
            case '*': {
                m_tokens.emplace_back(TokenType::MUL, '*', m_line, m_column);
                break;
            }
            case '/': {
                m_tokens.emplace_back(TokenType::DIV, '/', m_line, m_column);
                break;
            }
            case '%': {
                m_tokens.emplace_back(TokenType::MOD, '%', m_line, m_column);
                break;
            }
            case '=': {
                m_tokens.emplace_back(TokenType::EQUALS, '=', m_line, m_column);
                break;
            }
            case '?': {
                m_tokens.emplace_back(TokenType::ZERO_CHECK, '?', m_line, m_column);
                break;
            }
            case '!': {
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    m_tokens.emplace_back(TokenType::NOT_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    m_tokens.emplace_back(TokenType::STORE_VARIABLE, std::string{}, m_line, m_column);
                }
                break;
            }
            case '<': {
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    m_tokens.emplace_back(TokenType::LESS_THAN_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    m_tokens.emplace_back(TokenType::LESS_THAN, std::string{}, m_line, m_column);
                }
                break;
            }
            case '>': {
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    m_tokens.emplace_back(TokenType::GREATER_THAN_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    m_tokens.emplace_back(TokenType::GREATER_THAN, std::string{}, m_line, m_column);
                }
                break;
            }
            case '@': {
               // Skip whitespace after '@'
                while (peek().has_value() && std::isspace(peek().value())) {
                    consume();
                }

                if (!peek().has_value() || !std::isalpha(peek().value())) {
                    throwTokenizeError("[LEXER][ERROR]: Expected identifier after '@'", m_line, m_column);
                }

                std::string name;
                while (peek().has_value() && std::isalnum(peek().value())) {
                    name += consume();
                }

                m_tokens.emplace_back(TokenType::LOAD_VARIABLE, name, m_line, m_column);
                break;
            }
            // --- String literal ---
            case '"': {
                const size_t startLine   = m_line;
                const size_t startColumn = m_column - 1;

                buf.clear();

                while (peek().has_value() && peek().value() != '"') {
                    buf += consume();
                }
                if (peek().has_value()) {
                    consume(); // closing quote
                    m_tokens.emplace_back(TokenType::STR_LITERAL, buf, startLine, startColumn);
                    buf.clear();
                } else {
                    throwTokenizeError("[LEXER][ERROR]: Unterminated string literal", m_line, m_column);
                }
                break;
            }
            // --- Identifiers / keywords / numbers ---
            default: {
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    buf.clear();
                    buf += c;

                    while (peek().has_value()) {
                        char p = peek().value();
                        if (std::isalnum(static_cast<unsigned char>(p)) || p == '_' || p == '-') {
                            buf += consume();
                        } else {
                            break;
                        }
                    }

                    if (buf == "true") {
                        m_tokens.emplace_back(TokenType::BOOL_LITERAL, true, m_line, m_column);
                    }
                    else if (buf == "false") {
                        m_tokens.emplace_back(TokenType::BOOL_LITERAL, false, m_line, m_column);
                    }
                    else if (buf == "nil" || buf == "none") {
                        // Nil literal
                        m_tokens.emplace_back(TokenType::NIL_LITERAL, std::monostate{}, m_line, m_column);
                    }
                    else if (const auto it = keywords.find(buf); it != keywords.end()) {
                        m_tokens.emplace_back(it->second, std::string{}, m_line, m_column);
                    } else {
                        m_tokens.emplace_back(TokenType::IDENTIFIER, buf, m_line, m_column);
                    }
                    buf.clear();
                }
                else if (std::isdigit(static_cast<unsigned char>(c))) {
                    std::string prefix(1, c);
                    readNumber(std::move(prefix));
                }
                else {
                    throwTokenizeError("[LEXER][ERROR]: Unexpected character", m_line, m_column);
                }
            }
        }
    }
    return m_tokens;
}

std::optional<char> Tokenizer::peek(const size_t offset) const {
    if (m_pos + offset >= m_source.size()) {
        return std::nullopt;
    }

    return m_source.at(m_pos + offset);
}

char Tokenizer::consume() {
    const char c = m_source.at(m_pos++);
    if (c == '\n' || c == '\r') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

std::string Tokenizer::buildErrorContext(const size_t line, const size_t column) const {
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

    std::string out = "    " + lineStr + "\n    ";
    for (size_t i = 1; i < column; ++i) {
        out += ' ';
    }
    out += "^\n";
    return out;
}

[[noreturn]] void Tokenizer::throwTokenizeError(const std::string& message,const size_t line, const size_t column) const {
    throw EzException(EzError{
        .phase = EzErrorPhase::Tokenize,
        .message = message,
        .location = EzSourceLocation{m_moduleName, line, column},
        .snippet = buildErrorContext(line, column)
    });
}



