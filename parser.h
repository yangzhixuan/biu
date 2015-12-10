#ifndef __PARSER_H__
#define __PARSER_H__

#include <llvm/ADT/STLExtras.h>
#include <iostream>
#include <string>
#include <vector>
#include "typechecker.h"
#include "common.h"

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::unique_ptr;
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
    tok_error = -5
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
        virtual Type checkType(Enviroment & e) /*= 0*/;
};

class ExprAST : public ASTBase {
    public:
        virtual ~ExprAST() = 0;
        virtual Type parseType();
};

class AtomicAST : public ExprAST {
    public:
        AtomicAST(){};
        virtual ~AtomicAST() = 0;
};

class NumberAST : public AtomicAST {
    public:
        double value;
        NumberAST(double value) : value(value) {}
};

class StringAST : public AtomicAST {
    public:
        string str;
        StringAST(string str) : str(str) {}
};

class SymbolAST: public AtomicAST {
    public:
        string identifier;
        SymbolAST(string str) : identifier(str) {}
        Type parseType() override;
};

class FormAST : public ExprAST {
    public:
        virtual ~FormAST() = 0;

};

class ApplicationFormAST : public FormAST {
    public:
        vector<unique_ptr<ExprAST>> elements;
        //Type checkType(Enviroment & e) override;
        Type parseType() override;
};

class DefineFuncFormAST : public FormAST {
    public:
        unique_ptr<SymbolAST> name;
        unique_ptr<ExprAST> type;
        vector<pair<unique_ptr<SymbolAST>, unique_ptr<ExprAST>>> argList;
        vector<unique_ptr<ExprAST>> body;

        Type checkType(Enviroment & e) override;
};

class DefineVarFormAST : public FormAST {
    public:
        unique_ptr<SymbolAST> name;
        unique_ptr<ExprAST> value;
        Type checkType(Enviroment & e) override;
};

class IfFormAST : public FormAST {
    public:
        unique_ptr<ExprAST> condition, branch_true, branch_false;
};

class FormsAST : public ASTBase {
    public:
        vector<unique_ptr<FormAST>> forms;
        Type checkType(Enviroment& e) override;
};

// =====--------  Parser Part -------===================
// =====----------------------------===================

unique_ptr<FormsAST> parseForms();
unique_ptr<FormAST> parseForm();
unique_ptr<ExprAST> parseExpr();
unique_ptr<NumberAST> parseNumber();
unique_ptr<StringAST> parseString();
unique_ptr<SymbolAST> parseSymbol();
static void parserError(const string& info);
std::ostream& operator<<(std::ostream& out, const ParserError& err);
std::ostream& operator<<(std::ostream& out, const LexerError& err);
void initParser();

#endif
