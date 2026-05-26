#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <variant>
#include <functional>

namespace fs = std::filesystem;

#define LEAF_VERSION "1.0.0"

static void checkForUpdate() {
    std::cout << "  Leaf v" << LEAF_VERSION << "\n";
    std::cout << "  To update, run:\n";
    std::cout << "    irm https://raw.githubusercontent.com/777rupture/leaf/main/install.ps1 | iex\n\n";
    std::cout.flush();
}

// ─── Value type ──────────────────────────────────────────────────────────────

struct Value {
    enum class Type { Number, String, Bool, Null };
    Type type = Type::Null;
    double      num = 0;
    std::string str;
    bool        boolean = false;

    static Value fromNum(double n)         { Value v; v.type=Type::Number; v.num=n; return v; }
    static Value fromStr(std::string s)    { Value v; v.type=Type::String; v.str=std::move(s); return v; }
    static Value fromBool(bool b)          { Value v; v.type=Type::Bool;   v.boolean=b; return v; }
    static Value null()                    { return Value{}; }

    std::string toString() const {
        if (type==Type::Number) {
            if (num == (long long)num) return std::to_string((long long)num);
            return std::to_string(num);
        }
        if (type==Type::String)  return str;
        if (type==Type::Bool)    return boolean ? "true" : "false";
        return "null";
    }
    bool truthy() const {
        if (type==Type::Null)   return false;
        if (type==Type::Bool)   return boolean;
        if (type==Type::Number) return num != 0;
        if (type==Type::String) return !str.empty();
        return false;
    }
};

// ─── Interpreter ─────────────────────────────────────────────────────────────

class Interpreter {
public:
    std::unordered_map<std::string, Value> vars;
    std::vector<Token> tokens;
    size_t pos = 0;

    // signals
    bool doBreak = false;
    bool doSkip  = false;
    bool doReturn = false;
    Value returnVal;

    void run(const std::vector<Token>& toks) {
        tokens = toks;
        pos = 0;
        while (!atEnd()) execStatement();
    }

private:
    Token& cur()  { return tokens[pos]; }
    Token& peek(int off=1) { 
        size_t i = pos+off; 
        return i < tokens.size() ? tokens[i] : tokens.back(); 
    }
    bool atEnd()  { return cur().type == TokenType::EOF_TOKEN; }

    Token consume() { return tokens[pos++]; }

    bool check(TokenType t)     { return cur().type == t; }
    bool match(TokenType t)     { if(check(t)){pos++;return true;} return false; }
    void expect(TokenType t, const std::string& msg) {
        if (!check(t)) throw std::runtime_error("Line "+std::to_string(cur().line)+": "+msg+", got '"+cur().value+"'");
        pos++;
    }

    // ── String interpolation: replace <varname> with value ──────────────────
    std::string interpolate(const std::string& s) {
        std::string result;
        size_t i = 0;
        while (i < s.size()) {
            if (s[i] == '<') {
                size_t end = s.find('>', i);
                if (end != std::string::npos) {
                    std::string varname = s.substr(i+1, end-i-1);
                    auto it = vars.find(varname);
                    if (it != vars.end()) result += it->second.toString();
                    else result += s.substr(i, end-i+1);
                    i = end+1;
                    continue;
                }
            }
            result += s[i++];
        }
        return result;
    }

    // ── Execute one statement ─────────────────────────────────────────────
    void execStatement() {
        if (atEnd()) return;

        // skip stray closing braces (end of blocks)
        if (check(TokenType::RBRACE)) { pos++; return; }

        // print(...)
        if (check(TokenType::PRINT)) {
            pos++;
            expect(TokenType::LPAREN, "expected '(' after print");
            Value v = evalExpr();
            expect(TokenType::RPAREN, "expected ')' after print argument");
            std::string out = (v.type == Value::Type::String) ? interpolate(v.str) : v.toString();
            std::cout << out << "\n";
            std::cout.flush();
            return;
        }

        // return
        if (check(TokenType::RETURN)) {
            pos++;
            returnVal = evalExpr();
            doReturn = true;
            return;
        }

        // break
        if (check(TokenType::BREAK)) { pos++; doBreak = true; return; }

        // skip
        if (check(TokenType::SKIP)) { pos++; doSkip = true; return; }

        // if
        if (check(TokenType::IF)) { execIf(); return; }

        // while
        if (check(TokenType::WHILE)) { execWhile(); return; }

        // for i -> list
        if (check(TokenType::FOR)) { execFor(); return; }

        // fun
        if (check(TokenType::FUN)) { skipFun(); return; }

        // assignment or expression: IDENTIFIER = expr
        if (check(TokenType::IDENTIFIER) && peek().type == TokenType::ASSIGN) {
            std::string name = consume().value;
            pos++; // skip =
            vars[name] = evalExpr();
            return;
        }

        // fallback: skip unknown token
        pos++;
    }

    // ── Skip a fun definition (store for later — stub for now) ───────────
    void skipFun() {
        // skip until matching closing brace
        while (!atEnd() && !check(TokenType::LBRACE)) pos++;
        if (!atEnd()) skipBlock();
    }

    // ── Execute a { block } ───────────────────────────────────────────────
    void execBlock() {
        expect(TokenType::LBRACE, "expected '{'");
        while (!atEnd() && !check(TokenType::RBRACE)) {
            if (doBreak || doSkip || doReturn) break;
            execStatement();
        }
        if (check(TokenType::RBRACE)) pos++;
    }

    void skipBlock() {
        expect(TokenType::LBRACE, "expected '{'");
        int depth = 1;
        while (!atEnd() && depth > 0) {
            if (check(TokenType::LBRACE)) depth++;
            else if (check(TokenType::RBRACE)) depth--;
            pos++;
        }
    }

    // ── if / elseif / else ────────────────────────────────────────────────
    void execIf() {
        pos++; // skip 'if'
        Value cond = evalExpr();
        if (cond.truthy()) {
            execBlock();
            // skip remaining elseif/else
            while (check(TokenType::ELSEIF) || check(TokenType::ELSE)) {
                pos++;
                if (tokens[pos-1].type == TokenType::ELSE) { skipBlock(); break; }
                evalExpr(); // skip condition
                skipBlock();
            }
        } else {
            skipBlock();
            while (check(TokenType::ELSEIF)) {
                pos++;
                Value c2 = evalExpr();
                if (c2.truthy()) {
                    execBlock();
                    while (check(TokenType::ELSEIF) || check(TokenType::ELSE)) {
                        pos++;
                        if (tokens[pos-1].type == TokenType::ELSE) { skipBlock(); break; }
                        evalExpr();
                        skipBlock();
                    }
                    return;
                } else {
                    skipBlock();
                }
            }
            if (check(TokenType::ELSE)) { pos++; execBlock(); }
        }
    }

    // ── while ─────────────────────────────────────────────────────────────
    void execWhile() {
        pos++; // skip 'while'
        size_t condStart = pos;
        while (true) {
            pos = condStart;
            Value cond = evalExpr();
            if (!cond.truthy()) { skipBlock(); break; }
            execBlock();
            if (doBreak) { doBreak=false; break; }
            if (doSkip)  { doSkip=false; }
            if (doReturn) break;
        }
    }

    // ── for i -> list ─────────────────────────────────────────────────────
    void execFor() {
        pos++; // skip 'for'
        std::string iterVar = consume().value;
        expect(TokenType::ARROW, "expected '->' in for loop");
        std::string listVar = consume().value;
        size_t blockStart = pos;

        // get list value — for now support number ranges and identifier lists
        // Since we don't have real arrays yet, iterate over string tokens
        auto it = vars.find(listVar);
        if (it == vars.end()) { skipBlock(); return; }

        // if value is a string treat as single item
        Value listVal = it->second;
        std::vector<Value> items;
        if (listVal.type == Value::Type::String) {
            items.push_back(listVal);
        } else {
            items.push_back(listVal);
        }

        for (auto& item : items) {
            pos = blockStart;
            vars[iterVar] = item;
            execBlock();
            if (doBreak) { doBreak=false; break; }
            if (doSkip)  { doSkip=false; }
            if (doReturn) break;
        }
    }

    // ── Expression evaluator ──────────────────────────────────────────────
    Value evalExpr() { return evalOr(); }

    Value evalOr() {
        Value left = evalAnd();
        while (check(TokenType::OR)) { pos++; Value r=evalAnd(); left=Value::fromBool(left.truthy()||r.truthy()); }
        return left;
    }
    Value evalAnd() {
        Value left = evalEq();
        while (check(TokenType::AND)) { pos++; Value r=evalEq(); left=Value::fromBool(left.truthy()&&r.truthy()); }
        return left;
    }
    Value evalEq() {
        Value left = evalCmp();
        while (check(TokenType::EQ)||check(TokenType::NEQ)) {
            bool eq = check(TokenType::EQ); pos++;
            Value r=evalCmp();
            left = Value::fromBool(eq ? (left.toString()==r.toString()) : (left.toString()!=r.toString()));
        }
        return left;
    }
    Value evalCmp() {
        Value left = evalAdd();
        while (check(TokenType::LT)||check(TokenType::GT)||check(TokenType::LEQ)||check(TokenType::GEQ)) {
            TokenType op=cur().type; pos++;
            Value r=evalAdd();
            double a=left.num, b=r.num;
            if(op==TokenType::LT)  left=Value::fromBool(a<b);
            if(op==TokenType::GT)  left=Value::fromBool(a>b);
            if(op==TokenType::LEQ) left=Value::fromBool(a<=b);
            if(op==TokenType::GEQ) left=Value::fromBool(a>=b);
        }
        return left;
    }
    Value evalAdd() {
        Value left = evalMul();
        while (check(TokenType::PLUS)||check(TokenType::MINUS)) {
            bool add=check(TokenType::PLUS); pos++;
            Value r=evalMul();
            if(add && (left.type==Value::Type::String||r.type==Value::Type::String))
                left=Value::fromStr(left.toString()+r.toString());
            else
                left=Value::fromNum(add ? left.num+r.num : left.num-r.num);
        }
        return left;
    }
    Value evalMul() {
        Value left = evalUnary();
        while (check(TokenType::STAR)||check(TokenType::SLASH)||check(TokenType::PERCENT)) {
            TokenType op=cur().type; pos++;
            Value r=evalUnary();
            if(op==TokenType::STAR)    left=Value::fromNum(left.num*r.num);
            else if(op==TokenType::SLASH)   left=Value::fromNum(r.num!=0?left.num/r.num:0);
            else                            left=Value::fromNum(std::fmod(left.num,r.num));
        }
        return left;
    }
    Value evalUnary() {
        if(check(TokenType::BANG))  { pos++; return Value::fromBool(!evalUnary().truthy()); }
        if(check(TokenType::MINUS)) { pos++; return Value::fromNum(-evalUnary().num); }
        return evalPrimary();
    }
    Value evalPrimary() {
        if(check(TokenType::NUMBER)) {
            double n=std::stod(cur().value); pos++;
            return Value::fromNum(n);
        }
        if(check(TokenType::STRING)) {
            std::string s=cur().value; pos++;
            return Value::fromStr(s);
        }
        if(check(TokenType::TRUE))  { pos++; return Value::fromBool(true); }
        if(check(TokenType::FALSE)) { pos++; return Value::fromBool(false); }
        if(check(TokenType::NULL_TOKEN)) { pos++; return Value::null(); }
        if(check(TokenType::LPAREN)) {
            pos++;
            Value v=evalExpr();
            expect(TokenType::RPAREN,"expected ')'");
            return v;
        }
        if(check(TokenType::IDENTIFIER)) {
            std::string name=cur().value; pos++;
            // function call stub
            if(check(TokenType::LPAREN)) {
                pos++;
                std::vector<Value> args;
                while(!check(TokenType::RPAREN)&&!atEnd()) {
                    args.push_back(evalExpr());
                    match(TokenType::COMMA);
                }
                expect(TokenType::RPAREN,"expected ')'");
                // built-in: str() int() float()
                if(name=="str"   && args.size()==1) return Value::fromStr(args[0].toString());
                if(name=="int"   && args.size()==1) return Value::fromNum((long long)args[0].num);
                if(name=="float" && args.size()==1) return Value::fromNum(args[0].num);
                if(name=="bool"  && args.size()==1) return Value::fromBool(args[0].truthy());
                return Value::null();
            }
            auto it=vars.find(name);
            return it!=vars.end() ? it->second : Value::null();
        }
        // skip anything we don't understand
        pos++;
        return Value::null();
    }
};

// ─── Token name helper ────────────────────────────────────────────────────────

static const char* tokenTypeName(TokenType t) {
    switch(t){
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

// ─── CLI commands ─────────────────────────────────────────────────────────────

static void printHelp() {
    std::cout <<
        "\n"
        "  Leaf Programming Language\n"
        "\n"
        "  Usage:\n"
        "    leaf -create <filename.leaf>   Create a new Leaf source file\n"
        "    leaf -run    <filename.leaf>   Run a Leaf source file\n"
        "    leaf -tokens <filename.leaf>   Show token output for a file\n"
        "    leaf -help                     Show this help message\n"
        "    leaf -update                   Show how to update Leaf\n"
        "\n"
        "  Example:\n"
        "    leaf -create hello.leaf\n"
        "    leaf -run hello.leaf\n"
        "\n";
    std::cout.flush();
}

static int cmdCreate(const std::string& path) {
    if (fs::exists(path)) {
        std::cerr << "leaf: file already exists: " << path << "\n";
        return 1;
    }
    if (path.size() < 6 || path.substr(path.size()-5) != ".leaf") {
        std::cerr << "leaf: filename must end in .leaf\n";
        return 1;
    }
    std::ofstream file(path);
    if (!file) {
        std::cerr << "leaf: could not create file: " << path << "\n";
        return 1;
    }
    file << "print(\"hello\")\n";
    std::cout << "leaf: created " << path << "\n";
    std::cout.flush();
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
    try {
        Lexer lexer(buf.str());
        auto tokens = lexer.tokenize();
        Interpreter interp;
        interp.run(tokens);
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
    std::ostringstream buf;
    buf << file.rdbuf();
    try {
        Lexer lexer(buf.str());
        auto tokens = lexer.tokenize();
        std::cout << "=== LEAF TOKENS: " << path << " ===\n";
        for (const auto& tok : tokens)
            std::cout << "line " << tok.line << "\t" << tokenTypeName(tok.type) << "\t\"" << tok.value << "\"\n";
        std::cout << "\nTotal: " << tokens.size() << " tokens\n";
        std::cout.flush();
    } catch (const std::exception& e) {
        std::cerr << "leaf error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(true);
    setvbuf(stdout, nullptr, _IONBF, 0);

    if (argc < 2) { printHelp(); return 0; }

    std::string cmd = argv[1];
    if (cmd=="-help"||cmd=="--help"||cmd=="-h") { printHelp(); return 0; }
    if (cmd=="-update") { checkForUpdate(); return 0; }

    if (argc < 3) {
        std::cerr << "leaf: missing filename. Try: leaf -help\n";
        return 1;
    }

    std::string filename = argv[2];
    if      (cmd=="-create") return cmdCreate(filename);
    else if (cmd=="-run")    return cmdRun(filename);
    else if (cmd=="-tokens") return cmdTokens(filename);
    else {
        std::cerr << "leaf: unknown command '" << cmd << "'. Try: leaf -help\n";
        return 1;
    }
}