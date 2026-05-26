#include "lexer.h"
#include <stdexcept>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"fun",    TokenType::FUN},
    {"return", TokenType::RETURN},
    {"if",     TokenType::IF},
    {"elseif", TokenType::ELSEIF},
    {"else",   TokenType::ELSE},
    {"while",  TokenType::WHILE},
    {"for",    TokenType::FOR},
    {"break",  TokenType::BREAK},
    {"skip",   TokenType::SKIP},
    {"true",   TokenType::TRUE},
    {"false",  TokenType::FALSE},
    {"null",   TokenType::NULL_TOKEN},
    {"print",  TokenType::PRINT},
};

Lexer::Lexer(std::string source)
    : m_source(std::move(source)), m_pos(0), m_line(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        char c = current();

        if (std::isdigit(c))            { tokens.push_back(readNumber());              continue; }
        if (c == '"')                   { tokens.push_back(readString());              continue; }
        if (std::isalpha(c) || c == '_'){ tokens.push_back(readIdentifierOrKeyword()); continue; }

        advance();

        switch (c) {
            case '+': tokens.emplace_back(TokenType::PLUS,     "+", m_line); break;
            case '*': tokens.emplace_back(TokenType::STAR,     "*", m_line); break;
            case '/': tokens.emplace_back(TokenType::SLASH,    "/", m_line); break;
            case '%': tokens.emplace_back(TokenType::PERCENT,  "%", m_line); break;
            case '(': tokens.emplace_back(TokenType::LPAREN,   "(", m_line); break;
            case ')': tokens.emplace_back(TokenType::RPAREN,   ")", m_line); break;
            case '{': tokens.emplace_back(TokenType::LBRACE,   "{", m_line); break;
            case '}': tokens.emplace_back(TokenType::RBRACE,   "}", m_line); break;
            case '[': tokens.emplace_back(TokenType::LBRACKET, "[", m_line); break;
            case ']': tokens.emplace_back(TokenType::RBRACKET, "]", m_line); break;
            case ',': tokens.emplace_back(TokenType::COMMA,    ",", m_line); break;
            case '.': tokens.emplace_back(TokenType::DOT,      ".", m_line); break;

            case '-':
                if (!isAtEnd() && current() == '>') { advance(); tokens.emplace_back(TokenType::ARROW, "->", m_line); }
                else                                             { tokens.emplace_back(TokenType::MINUS, "-",  m_line); }
                break;
            case '=':
                if (!isAtEnd() && current() == '=') { advance(); tokens.emplace_back(TokenType::EQ,     "==", m_line); }
                else                                             { tokens.emplace_back(TokenType::ASSIGN, "=",  m_line); }
                break;
            case '!':
                if (!isAtEnd() && current() == '=') { advance(); tokens.emplace_back(TokenType::NEQ,  "!=", m_line); }
                else                                             { tokens.emplace_back(TokenType::BANG, "!",  m_line); }
                break;
            case '<':
                if (!isAtEnd() && current() == '=') { advance(); tokens.emplace_back(TokenType::LEQ, "<=", m_line); }
                else                                             { tokens.emplace_back(TokenType::LT,  "<",  m_line); }
                break;
            case '>':
                if (!isAtEnd() && current() == '=') { advance(); tokens.emplace_back(TokenType::GEQ, ">=", m_line); }
                else                                             { tokens.emplace_back(TokenType::GT,  ">",  m_line); }
                break;
            case '&':
                if (!isAtEnd() && current() == '&') { advance(); tokens.emplace_back(TokenType::AND, "&&", m_line); }
                else throw std::runtime_error("Line " + std::to_string(m_line) + ": unexpected '&'");
                break;
            case '|':
                if (!isAtEnd() && current() == '|') { advance(); tokens.emplace_back(TokenType::OR, "||", m_line); }
                else throw std::runtime_error("Line " + std::to_string(m_line) + ": unexpected '|'");
                break;

            default:
                throw std::runtime_error("Line " + std::to_string(m_line) + ": unknown character '" + c + "'");
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", m_line);
    return tokens;
}

char Lexer::current() const { return isAtEnd() ? '\0' : m_source[m_pos]; }
char Lexer::peek(int offset) const {
    size_t idx = m_pos + offset;
    return idx < m_source.size() ? m_source[idx] : '\0';
}
char Lexer::advance() { return m_source[m_pos++]; }
bool Lexer::isAtEnd() const { return m_pos >= m_source.size(); }

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = current();
        if (c == '\n')            { ++m_line; advance(); }
        else if (std::isspace(c)) { advance(); }
        else if (c == '/' && peek() == '/') {
            while (!isAtEnd() && current() != '\n') advance();
        }
        else break;
    }
}

Token Lexer::readNumber() {
    size_t start = m_pos;
    while (!isAtEnd() && std::isdigit(current())) advance();
    if (!isAtEnd() && current() == '.' && std::isdigit(peek())) {
        advance();
        while (!isAtEnd() && std::isdigit(current())) advance();
    }
    return { TokenType::NUMBER, m_source.substr(start, m_pos - start), m_line };
}

Token Lexer::readString() {
    advance(); // opening "
    size_t start = m_pos;
    while (!isAtEnd() && current() != '"') {
        if (current() == '\n') ++m_line;
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated string on line " + std::to_string(m_line));
    std::string val = m_source.substr(start, m_pos - start);
    advance(); // closing "
    return { TokenType::STRING, val, m_line };
}

Token Lexer::readIdentifierOrKeyword() {
    size_t start = m_pos;
    while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) advance();
    std::string word = m_source.substr(start, m_pos - start);
    auto it = KEYWORDS.find(word);
    return { (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER, word, m_line };
}