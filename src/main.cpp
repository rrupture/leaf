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
#include <algorithm>
#include <memory>
#include <functional>
#include <ctime>
#include <cstdlib>

namespace fs = std::filesystem;

#define LEAF_VERSION "1.1.0"

// ─── Forward declarations ─────────────────────────────────────────────────────
struct Value;
using ValuePtr = std::shared_ptr<Value>;

// ─── Value ────────────────────────────────────────────────────────────────────
struct Value {
    enum class Type { Number, String, Bool, Null, List, Function };
    Type type = Type::Null;

    double      num = 0;
    std::string str;
    bool        boolean = false;

    // list
    std::vector<ValuePtr> list;

    // function: param names + token range
    std::vector<std::string> params;
    size_t funStart = 0; // index of LBRACE in token stream
    std::vector<Token> funTokens; // captured tokens for the function body

    static ValuePtr fromNum(double n)       { auto v=std::make_shared<Value>(); v->type=Type::Number;   v->num=n;          return v; }
    static ValuePtr fromStr(std::string s)  { auto v=std::make_shared<Value>(); v->type=Type::String;   v->str=std::move(s); return v; }
    static ValuePtr fromBool(bool b)        { auto v=std::make_shared<Value>(); v->type=Type::Bool;     v->boolean=b;      return v; }
    static ValuePtr null()                  { return std::make_shared<Value>(); }
    static ValuePtr fromList(std::vector<ValuePtr> l) { auto v=std::make_shared<Value>(); v->type=Type::List; v->list=std::move(l); return v; }

    std::string toString() const {
        if (type==Type::Number) {
            if (num==(long long)num) return std::to_string((long long)num);
            std::ostringstream ss; ss<<num; return ss.str();
        }
        if (type==Type::String)   return str;
        if (type==Type::Bool)     return boolean?"true":"false";
        if (type==Type::Function) return "<fun>";
        if (type==Type::List) {
            std::string s="[";
            for (size_t i=0;i<list.size();i++){s+=list[i]->toString();if(i+1<list.size())s+=", ";}
            return s+"]";
        }
        return "null";
    }
    bool truthy() const {
        if (type==Type::Null)   return false;
        if (type==Type::Bool)   return boolean;
        if (type==Type::Number) return num!=0;
        if (type==Type::String) return !str.empty();
        if (type==Type::List)   return !list.empty();
        return false;
    }
};

// ─── Environment (scoped variables) ──────────────────────────────────────────
struct Env {
    std::unordered_map<std::string,ValuePtr> vars;
    std::shared_ptr<Env> parent;

    explicit Env(std::shared_ptr<Env> p=nullptr) : parent(p) {}

    ValuePtr get(const std::string& name) const {
        auto it=vars.find(name);
        if (it!=vars.end()) return it->second;
        if (parent) return parent->get(name);
        return Value::null();
    }
    void set(const std::string& name, ValuePtr val) {
        // if exists in parent scope, update there
        auto it=vars.find(name);
        if (it!=vars.end()) { it->second=val; return; }
        if (parent && parent->has(name)) { parent->set(name,val); return; }
        vars[name]=val;
    }
    void setLocal(const std::string& name, ValuePtr val) { vars[name]=val; }
    bool has(const std::string& name) const {
        if (vars.count(name)) return true;
        if (parent) return parent->has(name);
        return false;
    }
};

// ─── Interpreter ──────────────────────────────────────────────────────────────
class Interpreter {
public:
    std::vector<Token> tokens;
    size_t pos=0;
    std::shared_ptr<Env> globalEnv;

    bool doBreak=false, doSkip=false, doReturn=false;
    ValuePtr returnVal;

    Interpreter() : globalEnv(std::make_shared<Env>()) {}

    void run(const std::vector<Token>& toks) {
        tokens=toks; pos=0;
        while (!atEnd()) execStatement(globalEnv);
    }

    ValuePtr runLine(const std::vector<Token>& toks) {
        tokens=toks; pos=0;
        ValuePtr last=Value::null();
        while (!atEnd()) {
            size_t before=pos;
            try {
                last=evalExpr(globalEnv);
                if (!atEnd()) { pos=before; last=Value::null(); execStatement(globalEnv); }
            } catch(...) {
                pos=before;
                execStatement(globalEnv);
            }
        }
        return last;
    }

private:
    Token& cur()  { return tokens[pos]; }
    Token  peek(int off=1) const {
        size_t i=pos+off;
        return i<tokens.size()?tokens[i]:tokens.back();
    }
    bool atEnd()  { return cur().type==TokenType::EOF_TOKEN; }
    Token consume(){ return tokens[pos++]; }
    bool check(TokenType t){ return cur().type==t; }
    bool match(TokenType t){ if(check(t)){pos++;return true;} return false; }
    void expect(TokenType t,const std::string& msg){
        if(!check(t)) throw std::runtime_error("Line "+std::to_string(cur().line)+": "+msg+", got '"+cur().value+"'");
        pos++;
    }

    // ── String interpolation ─────────────────────────────────────────────
    std::string interpolate(const std::string& s, std::shared_ptr<Env> env) {
        std::string result;
        size_t i=0;
        while (i<s.size()) {
            if (s[i]=='<') {
                size_t end=s.find('>',i);
                if (end!=std::string::npos) {
                    std::string varname=s.substr(i+1,end-i-1);
                    auto val=env->get(varname);
                    result+=val->toString();
                    i=end+1; continue;
                }
            }
            result+=s[i++];
        }
        return result;
    }

    // ── Execute statement ────────────────────────────────────────────────
    void execStatement(std::shared_ptr<Env> env) {
        if (atEnd()) return;
        if (check(TokenType::RBRACE)) { pos++; return; }

        // print
        if (check(TokenType::PRINT)) {
            pos++;
            expect(TokenType::LPAREN,"expected '(' after print");
            auto v=evalExpr(env);
            expect(TokenType::RPAREN,"expected ')'");
            std::string out=(v->type==Value::Type::String)?interpolate(v->str,env):v->toString();
            std::cout<<out<<"\n"; std::cout.flush();
            return;
        }

        // return
        if (check(TokenType::RETURN)) {
            pos++;
            if (!check(TokenType::RBRACE)&&!atEnd()) returnVal=evalExpr(env);
            else returnVal=Value::null();
            doReturn=true; return;
        }

        // break / skip
        if (check(TokenType::BREAK)) { pos++; doBreak=true; return; }
        if (check(TokenType::SKIP))  { pos++; doSkip=true;  return; }

        // if
        if (check(TokenType::IF))    { execIf(env); return; }

        // while
        if (check(TokenType::WHILE)) { execWhile(env); return; }

        // for
        if (check(TokenType::FOR))   { execFor(env); return; }

        // fun definition
        if (check(TokenType::FUN))   { execFunDef(env); return; }

        // assignment: IDENTIFIER = expr  or  IDENTIFIER.method(...)
        if (check(TokenType::IDENTIFIER)) {
            // method call on variable: name.method(...)
            if (peek().type==TokenType::DOT) {
                // eval as expression statement
                evalExpr(env);
                return;
            }
            // assignment
            if (peek().type==TokenType::ASSIGN) {
                std::string name=consume().value;
                pos++; // skip =
                env->set(name,evalExpr(env));
                return;
            }
            // function call as statement
            if (peek().type==TokenType::LPAREN) {
                evalExpr(env);
                return;
            }
        }

        pos++; // skip unknown
    }

    // ── Fun definition ───────────────────────────────────────────────────
    void execFunDef(std::shared_ptr<Env> env) {
        pos++; // skip 'fun'
        std::string name=consume().value;
        expect(TokenType::LPAREN,"expected '(' after function name");
        std::vector<std::string> params;
        while (!check(TokenType::RPAREN)&&!atEnd()) {
            params.push_back(consume().value);
            match(TokenType::COMMA);
        }
        expect(TokenType::RPAREN,"expected ')'");

        // capture body tokens
        expect(TokenType::LBRACE,"expected '{'");
        int depth=1;
        size_t bodyStart=pos;
        std::vector<Token> body;
        while (!atEnd()&&depth>0) {
            if (check(TokenType::LBRACE)) depth++;
            else if (check(TokenType::RBRACE)) { depth--; if(depth==0) break; }
            body.push_back(cur());
            pos++;
        }
        expect(TokenType::RBRACE,"expected '}'");

        // add EOF
        body.push_back(Token(TokenType::EOF_TOKEN,"",0));

        auto fn=Value::null();
        fn->type=Value::Type::Function;
        fn->params=params;
        fn->funTokens=body;
        env->setLocal(name,fn);
    }

    // ── Call a function ──────────────────────────────────────────────────
    ValuePtr callFunction(ValuePtr fn, const std::vector<ValuePtr>& args, int line) {
        if (fn->type!=Value::Type::Function)
            throw std::runtime_error("Line "+std::to_string(line)+": not a function");

        auto fnEnv=std::make_shared<Env>(globalEnv);
        for (size_t i=0;i<fn->params.size();i++)
            fnEnv->setLocal(fn->params[i], i<args.size()?args[i]:Value::null());

        // run body tokens
        Interpreter sub;
        sub.globalEnv=globalEnv;
        sub.tokens=fn->funTokens;
        sub.pos=0;
        while (!sub.atEnd()) {
            sub.execStatement(fnEnv);
            if (sub.doReturn) return sub.returnVal;
            if (sub.doBreak||sub.doSkip) break;
        }
        return Value::null();
    }

    // ── Block ────────────────────────────────────────────────────────────
    void execBlock(std::shared_ptr<Env> env) {
        expect(TokenType::LBRACE,"expected '{'");
        while (!atEnd()&&!check(TokenType::RBRACE)) {
            if (doBreak||doSkip||doReturn) break;
            execStatement(env);
        }
        if (check(TokenType::RBRACE)) pos++;
    }

    void skipBlock() {
        expect(TokenType::LBRACE,"expected '{'");
        int depth=1;
        while (!atEnd()&&depth>0) {
            if (check(TokenType::LBRACE)) depth++;
            else if (check(TokenType::RBRACE)) depth--;
            pos++;
        }
    }

    // ── If / elseif / else ───────────────────────────────────────────────
    void execIf(std::shared_ptr<Env> env) {
        pos++;
        auto cond=evalExpr(env);
        if (cond->truthy()) {
            execBlock(env);
            while (check(TokenType::ELSEIF)||check(TokenType::ELSE)) {
                auto t=cur().type; pos++;
                if (t==TokenType::ELSE) { skipBlock(); break; }
                evalExpr(env); skipBlock();
            }
        } else {
            skipBlock();
            while (check(TokenType::ELSEIF)) {
                pos++;
                auto c2=evalExpr(env);
                if (c2->truthy()) {
                    execBlock(env);
                    while (check(TokenType::ELSEIF)||check(TokenType::ELSE)) {
                        auto t=cur().type; pos++;
                        if (t==TokenType::ELSE){skipBlock();break;}
                        evalExpr(env); skipBlock();
                    }
                    return;
                } else skipBlock();
            }
            if (check(TokenType::ELSE)) { pos++; execBlock(env); }
        }
    }

    // ── While ────────────────────────────────────────────────────────────
    void execWhile(std::shared_ptr<Env> env) {
        pos++;
        size_t condStart=pos;
        while (true) {
            pos=condStart;
            auto cond=evalExpr(env);
            if (!cond->truthy()) { skipBlock(); break; }
            auto loopEnv=std::make_shared<Env>(env);
            execBlock(loopEnv);
            if (doBreak){doBreak=false;break;}
            if (doSkip) {doSkip=false;continue;}
            if (doReturn) break;
        }
    }

    // ── For i -> list ────────────────────────────────────────────────────
    void execFor(std::shared_ptr<Env> env) {
        pos++;
        std::string iterVar=consume().value;
        expect(TokenType::ARROW,"expected '->' in for loop");
        auto listVal=evalExpr(env);
        size_t blockStart=pos;

        if (listVal->type!=Value::Type::List) {
            skipBlock(); return;
        }

        for (auto& item : listVal->list) {
            pos=blockStart;
            auto loopEnv=std::make_shared<Env>(env);
            loopEnv->setLocal(iterVar,item);
            execBlock(loopEnv);
            if (doBreak){doBreak=false;break;}
            if (doSkip) {doSkip=false;continue;}
            if (doReturn) break;
        }
    }

    // ── Expressions ──────────────────────────────────────────────────────
    ValuePtr evalExpr(std::shared_ptr<Env> env) { return evalOr(env); }

    ValuePtr evalOr(std::shared_ptr<Env> env) {
        auto left=evalAnd(env);
        while (check(TokenType::OR)){pos++; auto r=evalAnd(env); left=Value::fromBool(left->truthy()||r->truthy());}
        return left;
    }
    ValuePtr evalAnd(std::shared_ptr<Env> env) {
        auto left=evalEq(env);
        while (check(TokenType::AND)){pos++; auto r= evalEq(env); left=Value::fromBool(left->truthy()&&r->truthy());}
        return left;
    }
    ValuePtr evalEq(std::shared_ptr<Env> env) {
        auto left=evalCmp(env);
        while (check(TokenType::EQ)||check(TokenType::NEQ)) {
            bool eq=check(TokenType::EQ); pos++;
            auto r=evalCmp(env);
            left=Value::fromBool(eq?(left->toString()==r->toString()):(left->toString()!=r->toString()));
        }
        return left;
    }
    ValuePtr evalCmp(std::shared_ptr<Env> env) {
        auto left=evalAdd(env);
        while (check(TokenType::LT)||check(TokenType::GT)||check(TokenType::LEQ)||check(TokenType::GEQ)) {
            auto op=cur().type; pos++;
            auto r=evalAdd(env);
            double a=left->num,b=r->num;
            if(op==TokenType::LT)  left=Value::fromBool(a<b);
            if(op==TokenType::GT)  left=Value::fromBool(a>b);
            if(op==TokenType::LEQ) left=Value::fromBool(a<=b);
            if(op==TokenType::GEQ) left=Value::fromBool(a>=b);
        }
        return left;
    }
    ValuePtr evalAdd(std::shared_ptr<Env> env) {
        auto left=evalMul(env);
        while (check(TokenType::PLUS)||check(TokenType::MINUS)) {
            bool add=check(TokenType::PLUS); pos++;
            auto r=evalMul(env);
            if(add&&(left->type==Value::Type::String||r->type==Value::Type::String))
                left=Value::fromStr(left->toString()+r->toString());
            else left=Value::fromNum(add?left->num+r->num:left->num-r->num);
        }
        return left;
    }
    ValuePtr evalMul(std::shared_ptr<Env> env) {
        auto left=evalUnary(env);
        while (check(TokenType::STAR)||check(TokenType::SLASH)||check(TokenType::PERCENT)) {
            auto op=cur().type; pos++;
            auto r=evalUnary(env);
            if(op==TokenType::STAR)    left=Value::fromNum(left->num*r->num);
            else if(op==TokenType::SLASH) left=Value::fromNum(r->num!=0?left->num/r->num:0);
            else                           left=Value::fromNum(std::fmod(left->num,r->num));
        }
        return left;
    }
    ValuePtr evalUnary(std::shared_ptr<Env> env) {
        if(check(TokenType::BANG)) {pos++; return Value::fromBool(!evalUnary(env)->truthy());}
        if(check(TokenType::MINUS)){pos++; return Value::fromNum(-evalUnary(env)->num);}
        return evalPostfix(env);
    }

    // ── Postfix: method calls and index ──────────────────────────────────
    ValuePtr evalPostfix(std::shared_ptr<Env> env) {
        auto val=evalPrimary(env);
        while (true) {
            if (check(TokenType::DOT)) {
                pos++;
                std::string method=consume().value;
                expect(TokenType::LPAREN,"expected '(' after method name");
                std::vector<ValuePtr> args;
                while(!check(TokenType::RPAREN)&&!atEnd()){
                    args.push_back(evalExpr(env));
                    match(TokenType::COMMA);
                }
                expect(TokenType::RPAREN,"expected ')'");
                val=callMethod(val,method,args);
            } else if (check(TokenType::LBRACKET)) {
                pos++;
                auto idx=evalExpr(env);
                expect(TokenType::RBRACKET,"expected ']'");
                if(val->type==Value::Type::List){
                    int i=(int)idx->num;
                    if(i<0) i=(int)val->list.size()+i;
                    if(i>=0&&i<(int)val->list.size()) val=val->list[i];
                    else val=Value::null();
                } else val=Value::null();
            } else break;
        }
        return val;
    }

    // ── Method dispatch ───────────────────────────────────────────────────
    ValuePtr callMethod(ValuePtr obj, const std::string& method, const std::vector<ValuePtr>& args) {
        // List methods
        if (obj->type==Value::Type::List) {
            if (method=="push")  { if(!args.empty()) obj->list.push_back(args[0]); return Value::null(); }
            if (method=="pop")   { if(!obj->list.empty()){auto v=obj->list.back();obj->list.pop_back();return v;} return Value::null(); }
            if (method=="clear") { obj->list.clear(); return Value::null(); }
            if (method=="len")   { return Value::fromNum(obj->list.size()); }
            if (method=="has") {
                if (!args.empty()) {
                    for (auto& item:obj->list) if(item->toString()==args[0]->toString()) return Value::fromBool(true);
                }
                return Value::fromBool(false);
            }
        }
        // String methods
        if (obj->type==Value::Type::String) {
            std::string s=obj->str;
            if (method=="upper") { std::transform(s.begin(),s.end(),s.begin(),::toupper); return Value::fromStr(s); }
            if (method=="lower") { std::transform(s.begin(),s.end(),s.begin(),::tolower); return Value::fromStr(s); }
            if (method=="len")   { return Value::fromNum(s.size()); }
            if (method=="trim")  {
                size_t a=s.find_first_not_of(" \t\n\r");
                size_t b=s.find_last_not_of(" \t\n\r");
                return Value::fromStr(a==std::string::npos?"":s.substr(a,b-a+1));
            }
            if (method=="replace"&&args.size()>=2) {
                std::string from=args[0]->str, to=args[1]->str, res=s;
                size_t p=0;
                while((p=res.find(from,p))!=std::string::npos){res.replace(p,from.size(),to);p+=to.size();}
                return Value::fromStr(res);
            }
            if (method=="split") {
                std::string delim=args.empty()?" ":args[0]->str;
                std::vector<ValuePtr> parts;
                size_t p=0,f;
                while((f=s.find(delim,p))!=std::string::npos){parts.push_back(Value::fromStr(s.substr(p,f-p)));p=f+delim.size();}
                parts.push_back(Value::fromStr(s.substr(p)));
                return Value::fromList(parts);
            }
            if (method=="contains"&&!args.empty()) { return Value::fromBool(s.find(args[0]->str)!=std::string::npos); }
            if (method=="startswith"&&!args.empty()){ return Value::fromBool(s.rfind(args[0]->str,0)==0); }
            if (method=="endswith"&&!args.empty())  { auto& e=args[0]->str; return Value::fromBool(s.size()>=e.size()&&s.substr(s.size()-e.size())==e); }
        }
        return Value::null();
    }

    // ── Primary ───────────────────────────────────────────────────────────
    ValuePtr evalPrimary(std::shared_ptr<Env> env) {
        // Number
        if (check(TokenType::NUMBER)) {
            double n=std::stod(cur().value); pos++;
            return Value::fromNum(n);
        }
        // String
        if (check(TokenType::STRING)) {
            std::string s=cur().value; pos++;
            return Value::fromStr(s);
        }
        // Booleans / null
        if (check(TokenType::TRUE))       { pos++; return Value::fromBool(true); }
        if (check(TokenType::FALSE))      { pos++; return Value::fromBool(false); }
        if (check(TokenType::NULL_TOKEN)) { pos++; return Value::null(); }

        // List literal [...]
        if (check(TokenType::LBRACKET)) {
            pos++;
            std::vector<ValuePtr> items;
            while (!check(TokenType::RBRACKET)&&!atEnd()) {
                items.push_back(evalExpr(env));
                match(TokenType::COMMA);
            }
            expect(TokenType::RBRACKET,"expected ']'");
            return Value::fromList(items);
        }

        // Grouped (expr)
        if (check(TokenType::LPAREN)) {
            pos++;
            auto v=evalExpr(env);
            expect(TokenType::RPAREN,"expected ')'");
            return v;
        }

        // Identifier: variable, function call, or module
        if (check(TokenType::IDENTIFIER)) {
            std::string name=consume().value;

            // module: math.xxx or time.xxx or fs.xxx
            if (check(TokenType::DOT)) {
                pos++;
                std::string member=consume().value;
                std::vector<ValuePtr> args;
                if (check(TokenType::LPAREN)) {
                    pos++;
                    while(!check(TokenType::RPAREN)&&!atEnd()){
                        args.push_back(evalExpr(env));
                        match(TokenType::COMMA);
                    }
                    expect(TokenType::RPAREN,"expected ')'");
                }
                return callModule(name,member,args);
            }

            // function call
            if (check(TokenType::LPAREN)) {
                pos++;
                std::vector<ValuePtr> args;
                while(!check(TokenType::RPAREN)&&!atEnd()){
                    args.push_back(evalExpr(env));
                    match(TokenType::COMMA);
                }
                int line=cur().line;
                expect(TokenType::RPAREN,"expected ')'");

                // built-ins
                if(name=="str"  &&args.size()==1) return Value::fromStr(args[0]->toString());
                if(name=="int"  &&args.size()==1) return Value::fromNum((long long)args[0]->num);
                if(name=="float"&&args.size()==1) return Value::fromNum(args[0]->num);
                if(name=="bool" &&args.size()==1) return Value::fromBool(args[0]->truthy());
                if(name=="len"  &&args.size()==1) {
                    if(args[0]->type==Value::Type::List)   return Value::fromNum(args[0]->list.size());
                    if(args[0]->type==Value::Type::String) return Value::fromNum(args[0]->str.size());
                    return Value::fromNum(0);
                }
                if(name=="input") {
                    if(!args.empty()) { std::cout<<args[0]->toString(); std::cout.flush(); }
                    std::string line2; std::getline(std::cin,line2);
                    return Value::fromStr(line2);
                }
                if(name=="range") {
                    std::vector<ValuePtr> items;
                    int start=0, end=0, step=1;
                    if(args.size()==1){end=(int)args[0]->num;}
                    else if(args.size()==2){start=(int)args[0]->num;end=(int)args[1]->num;}
                    else if(args.size()>=3){start=(int)args[0]->num;end=(int)args[1]->num;step=(int)args[2]->num;}
                    if(step>0) for(int i=start;i<end;i+=step) items.push_back(Value::fromNum(i));
                    else       for(int i=start;i>end;i+=step) items.push_back(Value::fromNum(i));
                    return Value::fromList(items);
                }

                // user-defined function
                auto fn=env->get(name);
                if(fn->type==Value::Type::Function) return callFunction(fn,args,line);
                return Value::null();
            }

            return env->get(name);
        }

        // PRINT as expression
        if (check(TokenType::PRINT)) {
            pos++;
            expect(TokenType::LPAREN,"expected '('");
            auto v=evalExpr(env);
            expect(TokenType::RPAREN,"expected ')'");
            std::string out=(v->type==Value::Type::String)?interpolate(v->str,env):v->toString();
            std::cout<<out<<"\n"; std::cout.flush();
            return Value::null();
        }

        pos++;
        return Value::null();
    }

    // ── Module calls (math. time. fs.) ────────────────────────────────────
    ValuePtr callModule(const std::string& mod, const std::string& fn, const std::vector<ValuePtr>& args) {
        if (mod=="math") {
            if(fn=="sqrt" &&args.size()==1) return Value::fromNum(std::sqrt(args[0]->num));
            if(fn=="floor"&&args.size()==1) return Value::fromNum(std::floor(args[0]->num));
            if(fn=="ceil" &&args.size()==1) return Value::fromNum(std::ceil(args[0]->num));
            if(fn=="abs"  &&args.size()==1) return Value::fromNum(std::abs(args[0]->num));
            if(fn=="pow"  &&args.size()==2) return Value::fromNum(std::pow(args[0]->num,args[1]->num));
            if(fn=="sin"  &&args.size()==1) return Value::fromNum(std::sin(args[0]->num));
            if(fn=="cos"  &&args.size()==1) return Value::fromNum(std::cos(args[0]->num));
            if(fn=="tan"  &&args.size()==1) return Value::fromNum(std::tan(args[0]->num));
            if(fn=="log"  &&args.size()==1) return Value::fromNum(std::log(args[0]->num));
            if(fn=="random"&&args.empty())  return Value::fromNum((double)rand()/(double)RAND_MAX);
            if(fn=="pi"   &&args.empty())   return Value::fromNum(3.14159265358979323846);
        }
        if (mod=="fs") {
            if(fn=="read"&&args.size()==1) {
                std::ifstream f(args[0]->str);
                if(!f) return Value::null();
                std::ostringstream ss; ss<<f.rdbuf();
                return Value::fromStr(ss.str());
            }
            if(fn=="write"&&args.size()==2) {
                std::ofstream f(args[0]->str);
                if(!f) return Value::fromBool(false);
                f<<args[1]->str;
                return Value::fromBool(true);
            }
            if(fn=="exists"&&args.size()==1) return Value::fromBool(fs::exists(args[0]->str));
            if(fn=="delete"&&args.size()==1) { fs::remove(args[0]->str); return Value::null(); }
        }
        if (mod=="time") {
            if(fn=="now"&&args.empty()) return Value::fromNum((double)std::time(nullptr));
        }
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

// ─── CLI ─────────────────────────────────────────────────────────────────────
static void printHelp() {
    std::cout <<
        "\n"
        "  Leaf v" LEAF_VERSION "\n"
        "\n"
        "  Usage:\n"
        "    leaf -create <filename.leaf>   Create a new Leaf file\n"
        "    leaf -run    <filename.leaf>   Run a Leaf file\n"
        "    leaf -tokens <filename.leaf>   Show token debug output\n"
        "    leaf -repl                     Start interactive REPL\n"
        "    leaf -update                   How to update Leaf\n"
        "    leaf -help                     Show this help\n"
        "\n";
    std::cout.flush();
}

static void checkForUpdate() {
    std::cout << "  Leaf v" LEAF_VERSION "\n";
    std::cout << "  To update, run:\n";
    std::cout << "    irm https://raw.githubusercontent.com/rrupture/leaf/main/install.ps1 | iex\n\n";
    std::cout.flush();
}

static int cmdCreate(const std::string& path) {
    if (fs::exists(path)) { std::cerr<<"leaf: file already exists: "<<path<<"\n"; return 1; }
    if (path.size()<6||path.substr(path.size()-5)!=".leaf") { std::cerr<<"leaf: filename must end in .leaf\n"; return 1; }
    std::ofstream file(path);
    if (!file) { std::cerr<<"leaf: could not create file: "<<path<<"\n"; return 1; }
    file << "print(\"hello\")\n";
    std::cout<<"leaf: created "<<path<<"\n"; std::cout.flush();
    return 0;
}

static int cmdRun(const std::string& path) {
    if (!fs::exists(path)) { std::cerr<<"leaf: file not found: "<<path<<"\n"; return 1; }
    std::ifstream file(path);
    if (!file) { std::cerr<<"leaf: could not open: "<<path<<"\n"; return 1; }
    std::ostringstream buf; buf<<file.rdbuf();
    try {
        Lexer lexer(buf.str());
        Interpreter interp;
        interp.run(lexer.tokenize());
    } catch (const std::exception& e) {
        std::cerr<<"leaf error: "<<e.what()<<"\n"; return 1;
    }
    return 0;
}

static int cmdTokens(const std::string& path) {
    if (!fs::exists(path)) { std::cerr<<"leaf: file not found: "<<path<<"\n"; return 1; }
    std::ifstream file(path);
    std::ostringstream buf; buf<<file.rdbuf();
    try {
        Lexer lexer(buf.str());
        auto tokens=lexer.tokenize();
        std::cout<<"=== LEAF TOKENS: "<<path<<" ===\n";
        for (auto& tok:tokens)
            std::cout<<"line "<<tok.line<<"\t"<<tokenTypeName(tok.type)<<"\t\""<<tok.value<<"\"\n";
        std::cout<<"\nTotal: "<<tokens.size()<<" tokens\n";
        std::cout.flush();
    } catch (const std::exception& e) {
        std::cerr<<"leaf error: "<<e.what()<<"\n"; return 1;
    }
    return 0;
}

static int cmdRepl() {
    std::cout<<"  Leaf v"<<LEAF_VERSION<<" REPL — type 'exit' to quit\n\n";
    std::cout.flush();
    Interpreter interp;
    std::string line;
    while (true) {
        std::cout<<">> "; std::cout.flush();
        if (!std::getline(std::cin,line)) break;
        if (line=="exit"||line=="quit") break;
        if (line.empty()) continue;
        try {
            Lexer lexer(line);
            auto tokens=lexer.tokenize();
            auto result=interp.runLine(tokens);
            if (result->type!=Value::Type::Null)
                std::cout<<result->toString()<<"\n";
            std::cout.flush();
        } catch (const std::exception& e) {
            std::cerr<<"error: "<<e.what()<<"\n";
        }
    }
    std::cout<<"bye\n";
    return 0;
}

// ─── Entry point ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(true);
    setvbuf(stdout,nullptr,_IONBF,0);
    srand((unsigned)time(nullptr));

    if (argc<2) { printHelp(); return 0; }
    std::string cmd=argv[1];
    if (cmd=="-help"||cmd=="--help"||cmd=="-h") { printHelp(); return 0; }
    if (cmd=="-update") { checkForUpdate(); return 0; }
    if (cmd=="-repl")   { return cmdRepl(); }

    if (argc<3) { std::cerr<<"leaf: missing filename. Try: leaf -help\n"; return 1; }
    std::string filename=argv[2];
    if      (cmd=="-create") return cmdCreate(filename);
    else if (cmd=="-run")    return cmdRun(filename);
    else if (cmd=="-tokens") return cmdTokens(filename);
    else { std::cerr<<"leaf: unknown command '"<<cmd<<"'. Try: leaf -help\n"; return 1; }
}