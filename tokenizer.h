#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenType {
    PUSH, POP, SWAP, PEEK, DUP, DROP, PRINT, INT_LITERAL, STR_LITERAL,

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

#endif //TOKENIZER_H
