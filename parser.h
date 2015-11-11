#ifndef __PARSER_H__
#define __PARSER_H__

#include <llvm/ADT/STLExtras.h>
#include <iostream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::unique_ptr;

// =====--------  Lexer Part -------===================
// =====----------------------------===================
class Error{
    public:
        string msg;
        Error(string str) : msg(str) {}
};

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

};

class ExprAST : public ASTBase {
    public:
        virtual ~ExprAST() = 0;
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
};

class FormAST : public ExprAST {
    public:
        vector<unique_ptr<ExprAST>> elements;
};

class FormsAST : public ASTBase {
    public:
        vector<unique_ptr<FormAST>> forms;
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

#endif
