#include "tokenizer.h"

#include <iostream>
#include <bits/ostream.tcc>

tokenizer::tokenizer(std::string source): m_source(std::move(source)) {}

std::vector<Token> tokenizer::tokenize() {
    std::vector<Token> tokens;
    std::string buf;

    while (peek().has_value()) {
        switch (const char c = consume()) {
            case ' ':
            case '\n':
                break; // Skip whitespace
            case ';':
                while (peek().has_value() && peek().value() != '\n') consume();
                break; // Skip comments
            case '+': tokens.push_back({TokenType::ADD}); break;
            case '-': tokens.push_back({TokenType::SUB}); break;
            case '*': tokens.push_back({TokenType::MUL}); break;
            case '/': tokens.push_back({TokenType::DIV}); break;
            case '%': tokens.push_back({TokenType::MOD}); break;
            case '=': tokens.push_back({TokenType::EQUALS}); break;
            case '?': tokens.push_back({TokenType::ZERO_CHECK}); break;
            case '!':
                if (peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::NOT_EQUALS});
                }
                break;
            case '<':
                consume();
                if (peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::LESS_THAN_EQUALS});
                }
                else {
                    tokens.push_back({TokenType::LESS_THAN});
                }
                break;
            case '>':
                consume();
                if (peek().value() == '=') {
                    consume();
                    tokens.push_back({TokenType::GREATER_THAN_EQUALS});
                }
                else {
                    tokens.push_back({TokenType::GREATER_THAN});
                }
                break;
            case '"':
                buf += consume(); // Consume the opening quote
                while (peek().has_value() && peek().value() != '"') {
                    buf += consume();
                }
                if (peek().has_value()) {
                    consume();
                    tokens.push_back({TokenType::STR_LITERAL, buf});
                    buf.clear();
                } else {
                    std::cerr << "Unterminated string literal" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                if (std::isalpha(c)) {
                    buf += c;
                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buf += consume();
                    }
                    // if (buf == "push")       tokens.push_back({TokenType::PUSH});
                    if (buf == "pop")  tokens.push_back({TokenType::POP});
                    else if (buf == "drop") tokens.push_back({TokenType::DUP}); // Corrected DROP to DUP
                    else if (buf == "peek") tokens.push_back({TokenType::PEEK});
                    else if (buf == "swap") tokens.push_back({TokenType::SWAP});
                    else if (buf == "print")tokens.push_back({TokenType::PRINT});
                    else if (buf == "if")   tokens.push_back({TokenType::IF});
                    else if (buf == "else") tokens.push_back({TokenType::ELSE});
                    else if (buf == "while")tokens.push_back({TokenType::WHILE});
                    else if (buf == "end")  tokens.push_back({TokenType::END});
                    buf.clear();
                } else if (std::isdigit(c)) {
                    buf += c;
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buf += consume();
                    }
                    tokens.push_back({TokenType::INT_LITERAL, std::stoi(buf)});
                    buf.clear();
                } else {
                    std::cerr << "Unexpected character: " << c << std::endl;
                    exit(EXIT_FAILURE);
                }
        }
    }
    return tokens;
}

std::optional<char> tokenizer::peek(const size_t offset) const {
    if (m_pos + offset >= m_source.size()) {
        return std::nullopt;
    }

    return m_source.at(m_pos + offset);
}

char tokenizer::consume() {
    return m_source.at(m_pos++);
}


