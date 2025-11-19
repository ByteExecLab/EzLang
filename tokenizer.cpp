#include "tokenizer.h"

#include <iostream>
#include <unordered_map>

tokenizer::tokenizer(std::string source): m_source(std::move(source)) {}

auto tokenizer::readNumber(std::string prefix = "") {
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
        std::cerr << "Number parsing error: " << e.what() << std::endl;
        printErrorContext();
        std::exit(EXIT_FAILURE);
    }
}


/**
 * Tokenizes the input string, breaking it down into a sequence of tokens.
 *
 * This function iterates through the input string, identifying individual language elements
 * such as keywords, operators, literals, and identifiers.  It constructs a vector of
 * Token structures, where each token represents a meaningful unit of the source code.
 *
 * @return A vector of Token structures, representing the tokens found in the input string.
 */
std::vector<Token> tokenizer::tokenize() {
    std::string buf;

    bool lastWasValue = false; // Tracks if last emitted token was a "value"

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"dup", TokenType::DUP},
        {"swap", TokenType::SWAP},
        {"over", TokenType::OVER},
        {"nip", TokenType::NIP},
        {"tuck", TokenType::TUCK},
        {"drop", TokenType::DROP},
        {"const", TokenType::CONST},
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
                lastWasValue = false;
                break;
            }
            case ';': {
                // Skip comment until end of line
                while (peek().has_value() && peek().value() != '\n') consume();
                break;
            }
                // --- Operators / punctuation ---
            case '+': {
                m_tokens.emplace_back(TokenType::ADD, '+', m_line, m_column);
                lastWasValue = false;
                break;
            }
            case '-': {
                auto next = peek();
                bool nextIsDigit = next.has_value() &&
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
                lastWasValue = false;
                break;
            }
            case '/': {
                m_tokens.emplace_back(TokenType::DIV, '/', m_line, m_column);
                lastWasValue = false;
                break;
            }
            case '%': {
                m_tokens.emplace_back(TokenType::MOD, '%', m_line, m_column);
                lastWasValue = false;
                break;
            }
            case '=': {
                m_tokens.emplace_back(TokenType::EQUALS, '=', m_line, m_column);
                lastWasValue = false;
                break;
            }
            case '?': {
                m_tokens.emplace_back(TokenType::ZERO_CHECK, '?', m_line, m_column);
                lastWasValue = false;
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
                lastWasValue = false;
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
                lastWasValue = false;
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
                lastWasValue = false;
                break;
            }
            case '@': {
                m_tokens.emplace_back(TokenType::LOAD_VARIABLE, std::string{}, m_line, m_column);
                lastWasValue = false;
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
                    lastWasValue = true; // string literal is a value
                } else {
                    std::cerr << "Error at line " << m_line << ", column " << m_column
                              << ": Unterminated string literal" << std::endl;
                    printErrorContext();
                    std::exit(EXIT_FAILURE);
                }
                break;
            }
            // --- Identifiers / keywords / numbers ---
            default: {
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    buf.clear();
                    buf += c;
                    while (peek().has_value() && std::isalnum(static_cast<unsigned char>(peek().value()))) {
                        buf += consume();
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
                    std::cerr << "Error at line " << m_line << ", column " << m_column
                              << ": Unexpected character '" << c << "'\n";
                    printErrorContext();
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }
    return m_tokens;
}

/**
 * Look ahead in the input string by a specified offset without consuming characters.
 *
 * @param offset The number of characters to look ahead.  Must be non-negative.
 * @return An optional containing the character at the offset position, or std::nullptr
 * if the offset goes beyond the end of the input string.
 */
std::optional<char> tokenizer::peek(const size_t offset) const {
    if (m_pos + offset >= m_source.size()) {
        return std::nullopt;
    }

    return m_source.at(m_pos + offset);
}

/**
 * Consumes the next character from the input string and advances the position.
 *
 * @return The character that was consumed.
 */
char tokenizer::consume() {
    const char c = m_source.at(m_pos++);
    if (c == '\n' || c == '\r') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

void tokenizer::printErrorContext() const {
    size_t line_start = m_pos;
    while (line_start > 0 && m_source[line_start-1] != '\n') {
        line_start--;
    }
    size_t line_end = m_pos;
    while (line_end < m_source.size() && m_source[line_end] != '\n') {
        line_end++;
    }
    const std::string line_str = m_source.substr(line_start, line_end - line_start);
    std::cerr << line_str << std::endl;
    for (size_t i = 1; i < m_column; i++) std::cerr << " ";
    std::cerr << "^" << std::endl;
}



