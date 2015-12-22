#ifndef __PARSER_H__
#define __PARSER_H__

#include <llvm/ADT/STLExtras.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include "typechecker.h"
#include "common.h"
#include "codegen.h"

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using std::pair;

// =====--------  Lexer Part -------===================
// =====----------------------------===================
class LexerError : public Error {
    public:
        LexerError(string str) : Error(str) {}
};

enum Token {
    tok_eof = -1,
    tok_symbol = -2,
    tok_number = -3,
    tok_string = -4,
    tok_error = -5,
    tok_char = -6,
    tok_bool = -7
};

static int getToken();
static void lexerError(const string& info);
static string tok2str(int tok);

// =====--------  AST Part -------===================
// =====--------------------------===================

class ParserError : public Error {
    public:
        ParserError(string str) : Error(str) {}
};

class ASTBase {
    public:
        virtual ~ASTBase() = 0;
        virtual shared_ptr<BiuType> checkType(TypeEnvironment &e) /*= 0*/;
        virtual Value *codeGen(ValueEnvironment &e);
};

class ExprAST : public ASTBase {
    public:
        virtual ~ExprAST() = 0;
        virtual shared_ptr<BiuType> parseType();
        virtual void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded);
};

class AtomicAST : public ExprAST {
    public:
        AtomicAST(){};
        virtual ~AtomicAST() = 0;
};

class BoolAST : public AtomicAST {
    public:
        bool value;
        BoolAST(bool value) : value(value) {}
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
};

class NumberAST : public AtomicAST {
    public:
        double value;
        NumberAST(double value) : value(value) {}
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
};

class CharAST : public AtomicAST {
    public:
        char value;
        CharAST(char value) : value(value) {}
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
};

class StringAST : public AtomicAST {
    public:
        string str;
        StringAST(string str) : str(str) {}
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
};

class SymbolAST: public AtomicAST {
    public:
        string identifier;
        SymbolAST(string str) : identifier(str) {}
        shared_ptr<BiuType> parseType() override;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;

        shared_ptr<BiuType> symbolType;

};

class FormAST : public ExprAST {
    public:
        virtual ~FormAST() = 0;

};

class ApplicationFormAST : public FormAST {
    public:
        vector<unique_ptr<ExprAST>> elements;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        shared_ptr<BiuType> parseType() override;
        Value *codeGen(ValueEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;
};

class DefineFuncFormAST : public FormAST {
    public:
        unique_ptr<SymbolAST> name;
        unique_ptr<ExprAST> type;
        vector<pair<unique_ptr<SymbolAST>, unique_ptr<ExprAST>>> argList;
        vector<unique_ptr<ExprAST>> body;

        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;

    private:
        std::set<pair<string, shared_ptr<BiuType>>> freeVars;
        std::map<string, int> freeVarIndex;

        shared_ptr<FuncType> funType;

        llvm::Type *envType;
        std::vector<llvm::Type*> funcArgTypeIR;
        llvm::Type* retTypeIR;
        llvm::FunctionType* flatFuncType;
};

class DefineVarFormAST : public FormAST {
    public:
        unique_ptr<SymbolAST> name;
        unique_ptr<ExprAST> value;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;

    private:
        shared_ptr<BiuType> varType;
};

class ExternRawFormAST: public FormAST {
    public:
        unique_ptr<SymbolAST> name;
        unique_ptr<ExprAST> type;
        unique_ptr<StringAST> symbolName;

        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;

    private:
        shared_ptr<BiuType> varType;
};

class IfFormAST : public FormAST {
    public:
        unique_ptr<ExprAST> condition, branch_true, branch_false;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;
        Value *codeGen(ValueEnvironment &e) override;
    private:
        shared_ptr<BiuType> retType;
};

class MakeArrayAST : public FormAST {
    public:
        unique_ptr<ExprAST> eleTypeExpr, numEleExpr;

        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;
        Value *codeGen(ValueEnvironment &e) override;
    private:
        shared_ptr<BiuType> eleType;
        shared_ptr<ArrayType> arrType;
};

class SetIndexAST : public FormAST {
    public:
        unique_ptr<ExprAST> array, index, element;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;
        Value *codeGen(ValueEnvironment &e) override;
    private:
        shared_ptr<ArrayType> arrType;
};

class GetIndexAST : public FormAST {
    public:
        unique_ptr<ExprAST> array, index;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        void scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded) override;
        Value *codeGen(ValueEnvironment &e) override;
    private:
        shared_ptr<ArrayType> arrType;
};

class FormsAST : public ASTBase {
    public:
        vector<unique_ptr<FormAST>> forms;
        shared_ptr<BiuType> checkType(TypeEnvironment &e) override;
        Value *codeGen(ValueEnvironment &e) override;
};

// =====--------  Parser Part -------===================
// =====----------------------------===================

unique_ptr<FormsAST> parseForms();
unique_ptr<FormAST> parseForm();
unique_ptr<ExprAST> parseExpr();
unique_ptr<BoolAST> parseBool();
unique_ptr<NumberAST> parseNumber();
unique_ptr<CharAST> parseChar();
unique_ptr<StringAST> parseString();
unique_ptr<SymbolAST> parseSymbol();
static void parserError(const string& info);
std::ostream& operator<<(std::ostream& out, const ParserError& err);
std::ostream& operator<<(std::ostream& out, const LexerError& err);
void initParser();

#endif
