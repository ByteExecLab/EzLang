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
    PRINT, INT_LITERAL, STR_LITERAL, IDENTIFIER, FLOAT_LITERAL,
    CONST, LOAD_VARIABLE, STORE_VARIABLE,
    // Control
    IF, ELSE, WHILE, DO, FOR, END, ENDIF, CONTINUE, BREAK,
    ADD, SUB, MUL, DIV, MOD,
    EQUALS,NOT_EQUALS, LESS_THAN, LESS_THAN_EQUALS, GREATER_THAN, GREATER_THAN_EQUALS, ZERO_CHECK,
    TRACE,
};

/**
 * @struct Token
 * Represents a token with a specific type and an optional value. A token serves as a fundamental
 * unit of data within the tokenizer process, allowing identification and categorization of input
 * for further interpretation or analysis.
 */
struct Token {
    /**
     * Represents the type of token, which categorizes the token
     * and dictates its behavior in the tokenized structure. The type
     * is based on the TokenType enum and covers categories such as
     * instructions, literals, variables, control structures, math operations,
     * logical operators, and debugging symbols.
     */
    TokenType type;
    /**
     * Represents a union-like structure capable of holding a value of multiple different types:
     * int, double, or std::string. The primary use case of this type is to facilitate the storage
     * of diverse data types associated with tokens produced during the tokenization process.
     */
    std::variant<int, double, std::string> value;

    size_t line = 0;
    size_t column = 0;

    explicit Token(const TokenType t) : type(t), value(0) {} /**
     * Constructs a Token object with the specified token type and integer value.
     *
     * @param t The type of the token, represented by the `TokenType` enumeration.
     * @param v The integer value associated with the token.
     *
     * This constructor is typically used when creating tokens that require
     * an integer value, such as integer literals. The `type` member is assigned
     * the provided token type `t`, and the `value` member is set to the given
     * integer value `v`.
     */
    Token(const TokenType t, int v) : type(t), value(v) {}
    /**
     * Constructs a Token using a given TokenType and a double value.
     *
     * @param t The type of the token, represented as a `TokenType`.
     * @param v The numerical value associated with the token, represented as a `double`.
     */
    Token(const TokenType t, double v) : type(t), value(v) {}
    /**
     * @brief Constructs a new Token object with a specified TokenType and string value.
     *
     * This constructor initializes the Token's type and assigns a string value to it.
     * It is utilized to handle string literals, identifiers, or any other string-based
     * token representations during the tokenization process.
     *
     * @param t The TokenType of the token being created.
     * @param v The string value associated with the token.
     */
    Token(const TokenType t, const std::string &v) : type(t), value(v) {}
    /**
     * Constructs a Token object with a specified type and a value represented
     * as a variant of int, double, or std::string.
     *
     * @param t The type of the token, defined by the TokenType enumeration.
     * @param v The value associated with the token, as a std::variant<int, double, std::string>.
     */
    Token(const TokenType t, std::variant<int, double, std::string> v) : type(t), value(std::move(v)) {}

    Token(TokenType t, int v, size_t line_, size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(TokenType t, double v, size_t line_, size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(TokenType t, const std::string& v, size_t line_, size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(TokenType t, size_t line_, size_t col_)
        : type(t), value(0), line(line_), column(col_) {}

    Token(TokenType t, std::variant<int, double, std::string> v, size_t line_, size_t col_)
        : type(t), value(std::move(v)), line(line_), column(col_) {}
};

class tokenizer {
public:
    /**
     * Constructs a tokenizer object from the given source string.
     *
     * The constructor initializes the tokenizer with the provided source text,
     * which will later be processed to generate tokens during the tokenization process.
     *
     * @param source The source input string to be tokenized.
     */
    explicit tokenizer(std::string source);

    /**
     * Analyzes the input source string and converts it into a sequence of tokens.
     *
     * This method processes the source text passed to the tokenizer object and generates
     * a collection of `Token` objects by examining the text for keywords, symbols,
     * values, and other tokenizable elements. The tokens are categorized according to
     * their type using predefined rules and mappings.
     *
     * @return A vector of `Token` objects that represent the tokenized structure of
     *         the input source string.
     */
    std::vector<Token> tokenize();

private:
    /**
     * Peeks at the character in the source string at a specified offset from the current position.
     *
     * This method allows you to look ahead in the source string without advancing the current
     * position. If the offset goes beyond the end of the source string, it will return an empty
     * optional to indicate that no character is available at that location.
     *
     * @param offset The number of characters ahead of the current position to inspect.
     * @return An optional character at the specified offset, or an empty optional if the offset
     *         exceeds the source string's bounds.
     */
    [[nodiscard]]
    std::optional<char> peek(size_t offset = 0) const;


    /**
     * Consumes the next character from the source string, advancing the current position.
     *
     * This method retrieves the character at the current position in the source string,
     * increments the position pointer, and updates the line and column counters
     * accordingly. It is used to sequentially process the input text during tokenization.
     *
     * @return The character at the current position in the source string before it is advanced.
     */
    char consume();


    /**
     * Outputs the surrounding context of the current error in the input source.
     *
     * This method prints the line of source text where an error occurred,
     * followed by a caret ('^') marking the specific column where the issue was detected.
     * It aids in debugging by providing a visual representation of the error's location
     * within the source input.
     *
     * This function makes use of internal position tracking (`m_pos`, `m_line`, and `m_column`)
     * to extract and display the relevant portion of the source text associated with the error.
     */
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
        {TokenType::CONTINUE, "continue"},
        {TokenType::BREAK, "break"},
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
