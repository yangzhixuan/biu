#include <llvm/ADT/STLExtras.h>
#include <iostream>
#include <string>
#include "parser.h"

// =====--------  Lexer Part -------===================
// =====----------------------------===================
static double numVal;
static string symbolStr;
static string stringStr;

// gettok: return the next token from the standard input
static int getToken()
{
    static int lastChar = ' ';
    while(isspace(lastChar)) {
        lastChar = getchar();
    }

    if(isalpha(lastChar)) {
        // symbol: [A-Za-z_][A-Za-z0-9_]+
        symbolStr = "";
        do {
            symbolStr += lastChar;
            lastChar = getchar();
        } while(isalnum(lastChar) || lastChar == '_');
        return tok_symbol;
    }

    if(isdigit(lastChar)) {
        // number: [0-9]+(.[0-9]+)?
        string numStr = "";
        do {
            numStr += lastChar;
            lastChar = getchar();
        } while(isdigit(lastChar) || lastChar == '.');

        const char* endpoint;
        numVal = strtod(numStr.c_str(), nullptr);
        return tok_number;
    }

    if(lastChar == '"') {
        // string: \" is escaped
        stringStr = "";
        bool escaped = false;
        do {
            escaped = false;
            stringStr += lastChar;
            lastChar = getchar();
            if(lastChar == '\\') {
                lastChar = getchar();
                if(lastChar == '"') {
                    escaped = true;
                } else {
                    lexerError(string("Invalid escape character: ") + (char)lastChar);
                    return tok_error;
                }
            }
        } while((!escaped || lastChar != '"') && lastChar != EOF);

        if(lastChar == EOF) {
            lexerError("Nonterminated string");
            return tok_error;
        }

        return tok_string;
    }


    if(lastChar == ';') {
        // comment: skip until next line
        do {
            lastChar = getchar();
        } while(lastChar != EOF && lastChar != '\n' && lastChar != '\r');

        if(lastChar != EOF) {
            return getToken();
        }
    }


    if(lastChar == EOF) {
        return tok_eof;
    }

    int thisChar = lastChar;
    lastChar = getchar();
    return thisChar;
}

static string tok2str(int tok)
{
    switch(tok) {
        case tok_eof:
            return "tok_eof";
        case tok_number:
            return "tok_number";
        case tok_string:
            return "tok_string";
        case tok_symbol:
            return "tok_symbol";
        defualt:
            string ret = "";
            ret += tok;
            return ret;
    }
    return "unknown tok";
}

std::ostream& operator<<(std::ostream& out, const LexerError& err)
{
    out << "Lexer error: " << err.msg;
    return out;
}

static void lexerError(const string& info)
{
    throw(LexerError(info));
}

// =====--------  AST Part -------===================
// =====--------------------------===================
ASTBase::~ASTBase() {}
AtomicAST::~AtomicAST() {}
ExprAST::~ExprAST() {}

// =====--------  Parser Part ------===================
// =====----------------------------===================
static int curTok;
static int getNextToken() 
{
    curTok = getToken();
    return curTok;
}


unique_ptr<FormsAST> parseForms()
{
    auto forms = llvm::make_unique<FormsAST>();
    while(curTok != tok_eof) {
        forms->forms.push_back( parseForm() );
    }
    return forms;
}

unique_ptr<FormAST> parseForm()
{
    if(curTok != '(') {
        parserError(string("parseForm expects a '(', get: ") + tok2str(curTok));
        return nullptr;
    }
    getNextToken();
    auto form = llvm::make_unique<FormAST>();
    while(curTok != ')') {
        form->elements.push_back( parseExpr() );
    }
    getNextToken();
    return form;
}

unique_ptr<ExprAST> parseExpr()
{
    switch(curTok) {
        case '(':
            return parseForm();
        case tok_number:
            return parseNumber();
        case tok_string:
            return parseString();
        case tok_symbol:
            return parseSymbol();
        default:
            parserError(string("unexpected character: ") + tok2str(curTok));
            return nullptr;
    }
}

unique_ptr<NumberAST> parseNumber()
{
    auto result = llvm::make_unique<NumberAST>(numVal);
    getNextToken();
    return result;
}

unique_ptr<StringAST> parseString()
{
    auto result = llvm::make_unique<StringAST>(stringStr);
    getNextToken();
    return result;
}

unique_ptr<SymbolAST> parseSymbol()
{
    auto result = llvm::make_unique<SymbolAST>(symbolStr);
    getNextToken();
    return result;
}


static void parserError(const string& info) 
{
    throw(ParserError(info));
}


std::ostream& operator<<(std::ostream& out, const ParserError& err)
{
    out << "Parser error: " << err.msg;
    return out;
}

// =====--------  Main -------------===================
// =====----------------------------===================
int main(int argc, const char *argv[])
{
    try{
        getNextToken();
        auto ast = parseForms();
    }catch(const LexerError &e){
        cerr<<e<<std::endl;
    }catch(const ParserError &e){
        cerr<<e<<std::endl;
    }
    return 0;
}
