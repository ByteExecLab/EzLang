#include "tokenizer.h"

#include <iostream>
#include <unordered_map>

tokenizer::tokenizer(std::string source): m_source(std::move(source)) {}

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
    std::vector<Token> tokens;
    std::string buf;

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
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"do", TokenType::DO},
        {"end", TokenType::END},
        {"trace", TokenType::TRACE},
        {"endif", TokenType::ENDIF},
        {"continue", TokenType::CONTINUE},
        {"break", TokenType::BREAK},
        {"word", TokenType::WORD},
    };

    while (peek().has_value()) {
        switch (const char c = consume()) {
            case ' ':
            case '\n':
            case '\r':
                break;
            case ';':
                while (peek().has_value() && peek().value() != '\n') consume();
                break; // Skip comments
            case '+': tokens.emplace_back(Token{TokenType::ADD, '+', m_line, m_column}); break;
            case '-': tokens.emplace_back(TokenType::SUB, '-', m_line, m_column); break;
            case '*': tokens.emplace_back(TokenType::MUL, '*', m_line, m_column); break;
            case '/': tokens.emplace_back(TokenType::DIV, '/', m_line, m_column); break;
            case '%': tokens.emplace_back(TokenType::MOD, '%', m_line, m_column); break;
            case '=': tokens.emplace_back(TokenType::EQUALS, '=', m_line, m_column); break;
            case '?': tokens.emplace_back(TokenType::ZERO_CHECK, '?', m_line, m_column); break;
            case '!':
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    tokens.emplace_back(TokenType::NOT_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    tokens.emplace_back(TokenType::STORE_VARIABLE, std::string{}, m_line, m_column);
                }
                break;
            case '<':
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    tokens.emplace_back(TokenType::LESS_THAN_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    tokens.emplace_back(TokenType::LESS_THAN, std::string{}, m_line, m_column);
                }
                break;
            case '>':
                if (peek().has_value() && peek().value() == '=') {
                    consume();
                    tokens.emplace_back(TokenType::GREATER_THAN_EQUALS, std::string{}, m_line, m_column);
                }
                else {
                    tokens.emplace_back(TokenType::GREATER_THAN, std::string{}, m_line, m_column);
                }
                break;
            case '"': {
                const size_t startLine   = m_line;
                const size_t startColumn = m_column - 1;

                // Opening quote has already been consumed above
                buf.clear();

                // Read until closing quite
                while (peek().has_value() && peek().value() != '"') {
                    buf += consume();

                    // DEBUG LINE
                    // std::cout << buf << '\n';
                }
                if (peek().has_value()) {
                    consume(); // Consume the closing quote
                    tokens.emplace_back(TokenType::STR_LITERAL, buf, m_line, m_column);
                    buf.clear();
                } else {
                    std::cerr << "Error at line " << m_line << ", column " << m_column << ": Unterminated string literal" << std::endl;
                    printErrorContext();
                    exit(EXIT_FAILURE);
                }
                break;
            }
                case '@': {
                    tokens.emplace_back(TokenType::LOAD_VARIABLE, std::string{}, m_line, m_column);
                    break;
                }
            default:
                if (std::isalpha(c)) {
                   buf += c;
                    const size_t startLine   = m_line;
                    const size_t startColumn = m_column - 1;
                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buf += consume();
                    }

                   if (const auto it = keywords.find(buf); it != keywords.end()) {
                        tokens.emplace_back(it->second, std::string{}, startLine, startColumn);
                    }
                    else {
                        tokens.emplace_back(TokenType::IDENTIFIER, buf, startLine, startColumn);
                    }
                    buf.clear();
                }
                else if (isdigit(c)) {
                    buf.clear();
                    const size_t startLine   = m_line;
                    const size_t startColumn = m_column - 1;
                    buf += c;
                    bool isFloat = false;

                    // Read digits before the decimal point
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buf += consume();
                    }

                    // Check if the next char is '.' for float
                    if (peek().has_value() && peek().value() == '.') {
                        isFloat = true;
                        buf += consume();  // consume '.'

                        // Read digits after a decimal point
                        while (peek().has_value() && std::isdigit(peek().value())) {
                            buf += consume();
                        }
                    }

                    try {
                        if (isFloat) {
                            tokens.emplace_back(TokenType::FLOAT_LITERAL, std::stod(buf), startLine, startColumn);
                        } else {
                            tokens.emplace_back(TokenType::INT_LITERAL, std::stoi(buf), startLine, startColumn);
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Number parsing error: " << e.what() << std::endl;
                        printErrorContext();
                        exit(EXIT_FAILURE);
                    }

                    buf.clear();
                }
                else {
                    std::cerr << "Error at line " << m_line << ", column " << m_column << ": Unexpected character '" << c << "'" << std::endl;
                    printErrorContext();
                    exit(EXIT_FAILURE);
                }
        }
    }
    return tokens;
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



