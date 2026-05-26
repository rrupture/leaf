#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// ─── Token name helper ───────────────────────────────────────────────────────

static const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:     return "NUMBER";
        case TokenType::STRING:     return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::FUN:        return "FUN";
        case TokenType::RETURN:     return "RETURN";
        case TokenType::IF:         return "IF";
        case TokenType::ELSEIF:     return "ELSEIF";
        case TokenType::ELSE:       return "ELSE";
        case TokenType::WHILE:      return "WHILE";
        case TokenType::FOR:        return "FOR";
        case TokenType::ARROW:      return "ARROW";
        case TokenType::IN:         return "IN";
        case TokenType::BREAK:      return "BREAK";
        case TokenType::SKIP:       return "SKIP";
        case TokenType::TRUE:       return "TRUE";
        case TokenType::FALSE:      return "FALSE";
        case TokenType::NULL_TOKEN: return "NULL";
        case TokenType::PRINT:      return "PRINT";
        case TokenType::PLUS:       return "PLUS";
        case TokenType::MINUS:      return "MINUS";
        case TokenType::STAR:       return "STAR";
        case TokenType::SLASH:      return "SLASH";
        case TokenType::PERCENT:    return "PERCENT";
        case TokenType::ASSIGN:     return "ASSIGN";
        case TokenType::EQ:         return "EQ";
        case TokenType::NEQ:        return "NEQ";
        case TokenType::LT:         return "LT";
        case TokenType::GT:         return "GT";
        case TokenType::LEQ:        return "LEQ";
        case TokenType::GEQ:        return "GEQ";
        case TokenType::AND:        return "AND";
        case TokenType::OR:         return "OR";
        case TokenType::BANG:       return "BANG";
        case TokenType::LPAREN:     return "LPAREN";
        case TokenType::RPAREN:     return "RPAREN";
        case TokenType::LBRACE:     return "LBRACE";
        case TokenType::RBRACE:     return "RBRACE";
        case TokenType::LBRACKET:   return "LBRACKET";
        case TokenType::RBRACKET:   return "RBRACKET";
        case TokenType::COMMA:      return "COMMA";
        case TokenType::DOT:        return "DOT";
        case TokenType::EOF_TOKEN:  return "EOF";
        default:                    return "UNKNOWN";
    }
}

// ─── Commands ────────────────────────────────────────────────────────────────

static void printHelp() {
    std::cout <<
        "\n"
        "  🍃 Leaf Programming Language\n"
        "\n"
        "  Usage:\n"
        "    leaf -create <filename.leaf>   Create a new Leaf source file\n"
        "    leaf -run    <filename.leaf>   Run a Leaf source file\n"
        "    leaf -tokens <filename.leaf>   Show token output for a file\n"
        "    leaf -help                     Show this help message\n"
        "\n"
        "  Example:\n"
        "    leaf -create hello.leaf\n"
        "    leaf -run hello.leaf\n"
        "\n";
}

static int cmdCreate(const std::string& path) {
    // Don't overwrite existing files
    if (fs::exists(path)) {
        std::cerr << "leaf: file already exists: " << path << "\n";
        return 1;
    }

    // Make sure it ends in .leaf
    if (path.size() < 6 || path.substr(path.size() - 5) != ".leaf") {
        std::cerr << "leaf: filename must end in .leaf\n";
        return 1;
    }

    std::ofstream file(path);
    if (!file) {
        std::cerr << "leaf: could not create file: " << path << "\n";
        return 1;
    }

    // Write a starter template
    file <<
        "// " << path << "\n"
        "// Written in Leaf 🍃\n"
        "\n"
        "x = 10\n"
        "name = \"world\"\n"
        "\n"
        "fun greet(n) {\n"
        "    print(n)\n"
        "}\n"
        "\n"
        "if x > 5 {\n"
        "    greet(name)\n"
        "} else {\n"
        "    print(\"x is small\")\n"
        "}\n";

    std::cout << "leaf: created " << path << "\n";
    return 0;
}

static int cmdRun(const std::string& path) {
    if (!fs::exists(path)) {
        std::cerr << "leaf: file not found: " << path << "\n";
        return 1;
    }

    std::ifstream file(path);
    if (!file) {
        std::cerr << "leaf: could not open file: " << path << "\n";
        return 1;
    }

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // ── Placeholder interpreter ──────────────────────────────────────────
        // Right now the "interpreter" just walks the token stream and executes
        // print(...) statements so you get real output immediately.
        // Replace this block with a proper AST interpreter in Phase 3.
        // ────────────────────────────────────────────────────────────────────
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type == TokenType::PRINT) {
                // expect: PRINT LPAREN <value> RPAREN
                if (i + 3 < tokens.size() &&
                    tokens[i+1].type == TokenType::LPAREN &&
                    tokens[i+3].type == TokenType::RPAREN)
                {
                    std::cout << tokens[i+2].value << "\n";
                    i += 3;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "leaf error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

static int cmdTokens(const std::string& path) {
    if (!fs::exists(path)) {
        std::cerr << "leaf: file not found: " << path << "\n";
        return 1;
    }

    std::ifstream file(path);
    if (!file) {
        std::cerr << "leaf: could not open file: " << path << "\n";
        return 1;
    }

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        std::cout << "=== LEAF TOKENS: " << path << " ===\n";
        for (const auto& tok : tokens)
            std::cout << "line " << tok.line << "\t"
                      << tokenTypeName(tok.type) << "\t\""
                      << tok.value << "\"\n";
        std::cout << "\nTotal: " << tokens.size() << " tokens\n";

    } catch (const std::exception& e) {
        std::cerr << "leaf error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

// ─── Entry point ─────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "-help" || cmd == "--help" || cmd == "-h") {
        printHelp();
        return 0;
    }

    if (argc < 3) {
        std::cerr << "leaf: missing filename. Try: leaf -help\n";
        return 1;
    }

    std::string filename = argv[2];

    if      (cmd == "-create") return cmdCreate(filename);
    else if (cmd == "-run")    return cmdRun(filename);
    else if (cmd == "-tokens") return cmdTokens(filename);
    else {
        std::cerr << "leaf: unknown command '" << cmd << "'. Try: leaf -help\n";
        return 1;
    }
}