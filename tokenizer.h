#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenType: uint8_t {
    DUP, DROP, SWAP, OVER, NIP, TUCK,

    // System
    PRINT, INT_LITERAL, STR_LITERAL, IDENTIFIER, FLOAT_LITERAL,

    // Variables
    CONST, LOAD_VARIABLE, STORE_VARIABLE,

    // Control
    IF, ELSE, WHILE, DO, FOR, END,

    // Maths
    ADD, SUB, MUL, DIV, MOD,

    // Logical
    EQUALS,NOT_EQUALS, LESS_THAN, LESS_THAN_EQUALS, GREATER_THAN, GREATER_THAN_EQUALS, ZERO_CHECK,

    // Debug
    TRACE,
};

struct Token {
    TokenType type;
    std::variant<int, double, std::string> value;

    explicit Token(const TokenType t) : type(t), value(0) {} // Default int 0 for tokens without value
    Token(const TokenType t, int v) : type(t), value(v) {}
    Token(const TokenType t, double v) : type(t), value(v) {}
    Token(const TokenType t, const std::string &v) : type(t), value(v) {}
    Token(const TokenType t, std::variant<int, double, std::string> v) : type(t), value(std::move(v)) {}
};

class tokenizer {

public:
    explicit tokenizer(std::string source);

    std::vector<Token> tokenize();

private:
    [[nodiscard]]
    std::optional<char> peek(size_t offset = 0) const;
    char consume();

    void printErrorContext() const;

private:
    std::string m_source;
    size_t m_pos = 0;
    size_t m_line = 1;
    size_t m_column = 1;
};

inline std::string tokenTypeToString(const TokenType type) {
    static std::map<TokenType, std::string> typeStrings = {
        {TokenType::DUP, "DUP"},
        {TokenType::DROP, "DROP"},
        {TokenType::SWAP, "SWAP"},
        {TokenType::OVER, "OVER"},
        {TokenType::NIP, "NIP"},
        {TokenType::TUCK, "TUCK"},
        //
        {TokenType::IDENTIFIER, "IDENTIFIER"},
        {TokenType::CONST, "CONST"},
        {TokenType::LOAD_VARIABLE, "LOAD_VARIABLE"},
        //
        {TokenType::PRINT, "PRINT"},
        {TokenType::INT_LITERAL, "INT_LITERAL"},
        {TokenType::STR_LITERAL, "STR_LITERAL"},
        {TokenType::IF, "IF"},
        {TokenType::ELSE, "ELSE"},
        {TokenType::WHILE, "WHILE"},
        {TokenType::DO, "DO"},
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
        {TokenType::ZERO_CHECK, "ZERO_CHECK"},

        // Debug
        {TokenType::TRACE, "TRACE"},
    };

    if (const auto it = typeStrings.find(type); it != typeStrings.end()) {
        return it->second;
    }

    return "UNKNOWN_TOKEN_TYPE";
}

#endif //TOKENIZER_H
