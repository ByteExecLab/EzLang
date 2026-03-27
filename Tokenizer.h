#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "Common.h"

enum class TokenType: uint8_t {
    DUP, DROP, SWAP, OVER, NIP, TUCK,

    // System
    PRINT, INT_LITERAL, STR_LITERAL, BOOL_LITERAL, IDENTIFIER, FLOAT_LITERAL, NIL_LITERAL,

    // Variables
    CONST, VAR, LOAD_VARIABLE, STORE_VARIABLE,

    // Locals
    LET,
    SET,

    // Logical
    EQUALS,NOT_EQUALS, LESS_THAN, LESS_THAN_EQUALS, GREATER_THAN, GREATER_THAN_EQUALS, ZERO_CHECK,
    AND, OR, NOT,

    // Control
    IF, ELSE, WHILE, DO, FOR, END, ENDIF, CONTINUE, BREAK, RETURN,

    // Maths
    ADD, SUB, MUL, DIV, MOD,

    // Debug
    TRACE,

    // Aggregates
    ARRAY_START, ARRAY_END,
    STRUCT_START, STRUCT_END,

    // Arrays
    ARRAY_LEN, ARRAY_GET, ARRAY_SET,

    // Structs
    STRUCT_GET, STRUCT_SET, STRUCT_ACCESS,

    // Words
    WORD,
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
    StackValue value;

    size_t line = 0;
    size_t column = 0;

    explicit Token(const TokenType t) : type(t), value(std::monostate{}) {} /**
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
    Token(const TokenType t, StackValue v) : type(t), value(std::move(v)) {}

    Token(const TokenType t, int v, const size_t line_, const size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(const TokenType t, double v, const size_t line_, const size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(const TokenType t, const std::string& v, const size_t line_, const size_t col_)
        : type(t), value(v), line(line_), column(col_) {}

    Token(const TokenType t, const size_t line_, const size_t col_)
        : type(t), value(0), line(line_), column(col_) {}

    Token(const TokenType t, StackValue v, const size_t line_, const size_t col_)
        : type(t), value(std::move(v)), line(line_), column(col_) {}
};

class Tokenizer {
public:
    /**
     * Constructs a tokenizer object from the given source string.
     *
     * The constructor initializes the tokenizer with the provided source text,
     * which will later be processed to generate tokens during the tokenization process.
     *
     * @param source The source input string to be tokenized.
     */
    explicit Tokenizer(std::string source);

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

    /**
     * Reads a numeric sequence from the input starting with an optional prefix.
     * This method identifies numbers either as integers or floating-point literals,
     * processes the input, and stores the corresponding token in the token list.
     * If parsing fails, it will output an error message and terminate the program.
     *
     * @param prefix An optional initial string to prepend to the number being read.
     *               This can be used to supply a starting context for the numeric value.
     * @return void This method does not return a value, but updates the internal
     *              token list with an integer or floating-point number token.
     */
    auto readNumber(std::string prefix);

private:
    /**
     * @variable m_tokens
     * A collection of `Token` objects representing the tokenized elements of the source input.
     *
     * This member variable stores the result of the tokenization process, where each `Token`
     * represents a distinct unit of meaningful data identified from the source string. The
     * tokens in this vector are categorized and structured according to the rules defined
     * by the tokenizer, facilitating further processing or analysis of the input.
     */
    std::vector<Token> m_tokens;

    /**
     * @variable m_source
     * The source string to be tokenized.
     *
     * This member variable holds the input text provided to the tokenizer. It serves as
     * the primary data source that the tokenizer processes to generate tokens. The string
     * is analyzed character by character, and its contents are used for token generation,
     * error context output, and lookup mechanisms during the tokenization process.
     */
    std::string m_source;

    /**
     * @variable m_pos
     * Tracks the current position within the source string being tokenized.
     *
     * This member variable maintains an index into the source string (`m_source`) that indicates
     * the character currently being processed or that will be processed next during the tokenization
     * process. It is incremented as characters are consumed or inspected, enabling sequential
     * navigation through the input text. The position is used in various internal methods such as
     * peeking and consuming characters, as well as generating error context.
     */
    size_t m_pos = 0;

    /**
     * @variable m_line
     * Tracks the current line number being processed in the tokenizer. This is useful for error reporting
     * and debugging by providing context about the location within the input data.
     */
    size_t m_line = 1;

    /**
     * @var m_column
     * Represents the current column index being processed. This value is typically used
     * to track the horizontal position in a data stream or input, aiding in parsing or
     * error reporting by providing column-level context.
     */
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
        {TokenType::VAR, "VAR"},
        {TokenType::LOAD_VARIABLE, "LOAD_VARIABLE"},
        //
        {TokenType::PRINT, "PRINT"},
        {TokenType::INT_LITERAL, "INT_LITERAL"},
        {TokenType::STR_LITERAL, "STR_LITERAL"},
        {TokenType::BOOL_LITERAL, "BOOL_LITERAL"},
        {TokenType::NIL_LITERAL, "nil"},
        {TokenType::IF, "IF"},
        {TokenType::ELSE, "ELSE"},
        {TokenType::ENDIF, "ENDIF"},
        {TokenType::AND, "AND"},
        {TokenType::OR, "OR"},
        {TokenType::NOT, "NOT"},
        {TokenType::WHILE, "WHILE"},
        {TokenType::DO, "DO"},
        {TokenType::CONTINUE, "CONTINUE"},
        {TokenType::BREAK, "BREAK"},
        {TokenType::RETURN, "RETURN"},
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

        // Frame/function based variable functions
        {TokenType::LET, "LET"},
        {TokenType::SET, "SET"},

        // Arrays & Structs
        {TokenType::ARRAY_START, "ARRAY_START"},
        {TokenType::ARRAY_END, "ARRAY_END"},
        {TokenType::STRUCT_START, "STRUCT_START"},
        {TokenType::STRUCT_END, "STRUCT_END"},

        {TokenType::ARRAY_LEN,    "ARRAY_LEN"},
        {TokenType::ARRAY_GET,    "ARRAY_GET"},
        {TokenType::ARRAY_SET,    "ARRAY_SET"},
        {TokenType::STRUCT_GET,   "STRUCT_GET"},
        {TokenType::STRUCT_SET,   "STRUCT_SET"},
        {TokenType::STRUCT_ACCESS,"STRUCT_ACCESS"},

        // Debug
        {TokenType::TRACE, "TRACE"},

        {TokenType::WORD, "WORD"}
    };

    if (const auto it = typeStrings.find(type); it != typeStrings.end()) {
        return it->second;
    }

    return "UNKNOWN_TOKEN_TYPE";
}

#endif //TOKENIZER_H
