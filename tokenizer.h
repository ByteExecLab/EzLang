#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenType {
    DUP, DROP, SWAP, OVER, NIP,

    PRINT, INT_LITERAL, STR_LITERAL,

    // Control
    IF, ELSE, WHILE, FOR, END,

    // Maths
    ADD, SUB, MUL, DIV, MOD,

    // Logical
    EQUALS, NOT_EQUALS, LESS_THAN, LESS_THAN_EQUALS, GREATER_THAN, GREATER_THAN_EQUALS, ZERO_CHECK
};

struct Token {
    TokenType type;
    std::variant<int, std::string> value;
};

class tokenizer {

public:
    explicit tokenizer(std::string source);

    std::vector<Token> tokenize();

private:
    [[nodiscard]]
    std::optional<char> peek(size_t offset = 0) const;
    char consume();

private:
    std::string m_source;
    size_t m_pos = 0;
};

inline std::string tokenTypeToString(const TokenType type) {
    static std::map<TokenType, std::string> typeStrings = {
        {TokenType::DUP, "DUP"},
        {TokenType::DROP, "DROP"},
        {TokenType::SWAP, "SWAP"},
        {TokenType::OVER, "OVER"},
        {TokenType::NIP, "NIP"},
        //
        {TokenType::PRINT, "PRINT"},
        {TokenType::INT_LITERAL, "INT_LITERAL"},
        {TokenType::STR_LITERAL, "STR_LITERAL"},
        {TokenType::IF, "IF"},
        {TokenType::ELSE, "ELSE"},
        {TokenType::WHILE, "WHILE"},
        {TokenType::FOR, "FOR"},
        {TokenType::END, "END"},
        {TokenType::ADD, "ADD"},
        {TokenType::SUB, "SUB"},
        {TokenType::MUL, "MUL"},
        {TokenType::DIV, "DIV"},
        {TokenType::MOD, "MOD"},
        {TokenType::EQUALS, "EQUALS"},
        {TokenType::NOT_EQUALS, "NOT_EQUALS"},
        {TokenType::LESS_THAN, "LESS_THAN"},
        {TokenType::LESS_THAN_EQUALS, "LESS_THAN_EQUALS"},
        {TokenType::GREATER_THAN, "GREATER_THAN"},
        {TokenType::GREATER_THAN_EQUALS, "GREATER_THAN_EQUALS"},
        {TokenType::ZERO_CHECK, "ZERO_CHECK"}
    };
    return typeStrings[type];
}

#endif //TOKENIZER_H
