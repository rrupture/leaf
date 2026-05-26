#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Literals
    NUMBER,
    STRING,
    IDENTIFIER,

    // Keywords
    FUN,
    RETURN,
    IF,
    ELSEIF,
    ELSE,
    WHILE,
    FOR,
    IN,         // -> (for loop arrow)
    BREAK,
    SKIP,
    TRUE,
    FALSE,
    NULL_TOKEN,

    // Print (built-in for now)
    PRINT,

    // Arithmetic
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    // Comparison
    ASSIGN,     // =
    EQ,         // ==
    NEQ,        // !=
    LT,         // <
    GT,         // >
    LEQ,        // <=
    GEQ,        // >=

    // Logical
    AND,        // &&
    OR,         // ||
    BANG,       // !

    // Delimiters
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    LBRACKET,   // [
    RBRACKET,   // ]
    COMMA,      // ,
    DOT,        // .
    ARROW,      // ->

    // Special
    EOF_TOKEN,
    UNKNOWN
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;

    Token(TokenType t, std::string v, int l)
        : type(t), value(std::move(v)), line(l) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string m_source;
    size_t      m_pos;
    int         m_line;

    char current() const;
    char peek(int offset = 1) const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
};