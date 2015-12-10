#include <llvm/ADT/STLExtras.h>
#include <iostream>
#include <string>
#include "parser.h"

// =====--------  Lexer Part -------===================
// =====----------------------------===================
static double numVal;
static string symbolStr;
static string stringStr;

static int validSymbolChar(int c)
{
    return isalpha(c) || c == '_' || c == '+' || c == '-'
        || c == '*' || c == '/' || c == '?' || c == '=' || c == '<' || c == '>'
        || c == ':' || c == '!';
}

// gettok: return the next token from the standard input
static int getToken()
{
    static int lastChar = ' ';
    while(isspace(lastChar)) {
        lastChar = getchar();
    }

    if(validSymbolChar(lastChar)) {
        // symbol: [SymbolChar][SymbolChar,0-9]+
        symbolStr = "";
        do {
            symbolStr += lastChar;
            lastChar = getchar();
        } while(validSymbolChar(lastChar) || isdigit(lastChar));
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
        } while((!escaped && lastChar != '"') && lastChar != EOF);

        if(lastChar == '"') {
            lastChar = getchar();
        }

        if(lastChar == EOF) {
            lexerError(string("Nonterminated string: ") + stringStr);
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
            return "tok_symbol(" + symbolStr + ")";
        default:
            string ret = "";
            ret += tok;
            return ret;
    }
    return "unknown tok(" + std::to_string(tok) + ")";
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
FormAST::~FormAST() {}
AtomicAST::~AtomicAST() {}
ExprAST::~ExprAST() {}

// =====--------  Parser Part ------===================
// =====----------------------------===================
static int curTok;
static int getNextToken() 
{
    printf("consume: %s\n", tok2str(curTok).c_str());
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

    if(curTok == tok_symbol && symbolStr == "define") {
        getNextToken();
        if(curTok == tok_symbol) {
            // variable definition
            auto form = llvm::make_unique<DefineVarFormAST>();
            // TODO add define-var type annotating
            form->name = parseSymbol();
            form->value = parseExpr();
            if(curTok != ')') {
                parserError(string("parseForm: define-var-form expect a ')', get: ") + tok2str(curTok));
                return nullptr;
            }
            getNextToken();
        return std::move(form);
            return move(form);
        } else if(curTok == '(') {
            // function definition
            auto form = llvm::make_unique<DefineFuncFormAST>();
            getNextToken();


            if(curTok == tok_symbol) {
                form->name = parseSymbol();
            } else if (curTok == '(') {
                getNextToken();
                form->name = parseSymbol();
                form->type = parseExpr();
                if(curTok != ')') {
                    parserError(string("parseForm expects ')', get: ") + tok2str(curTok));
                    return nullptr;
                }
                getNextToken();
            } else {
                parserError(string("parseForm expects function name or name/type pair, get: ") + tok2str(curTok));
                return nullptr;
            }


            // parse arglist
            while(curTok == '(') {
                getNextToken();
                auto argName = parseSymbol();
                auto argType = parseExpr();
                form->argList.push_back(make_pair(std::move(argName), std::move(argType)));
                if(curTok != ')') {
                    parserError(string("parseForm: argument pair expects a ')', get: ") + tok2str(curTok));
                    return nullptr;
                }
                getNextToken();
            }

            if(curTok != ')') {
                parserError(string("parseForm expect a ')', get: ") + tok2str(curTok));
                return nullptr;
            }
            getNextToken();

            while(curTok != ')') {
                form->body.push_back( parseExpr() );
            }
            getNextToken();
            return std::move(form);
        } else {
            parserError(string("invalid define-form, next token: ") + tok2str(curTok));
            return nullptr;
        }

    } else if(curTok == tok_symbol && symbolStr == "if") {
        // if-form
        getNextToken();
        auto form = llvm::make_unique<IfFormAST>();
        form->condition = parseExpr();
        form->branch_true = parseExpr();
        if(curTok != ')') {
            form->branch_false = parseExpr();
        }

        if(curTok != ')') {
            parserError(string("parseForm: if-form expect a ')', get: ") + tok2str(curTok));
            return nullptr;
        }
        getNextToken();
        return std::move(form);
    } else if(curTok == tok_symbol && symbolStr == "extern") {
        // extern-form
        getNextToken();
        auto form = llvm::make_unique<ExternFormAST>();
        form->name = parseSymbol();
        form->type = parseExpr();
        if(curTok != ')') {
            parserError(string("parseForm: extern-form expect a ')', get: ") + tok2str(curTok));
            return nullptr;
        }
        getNextToken();
        return std::move(form);
    } else {
        auto form = llvm::make_unique<ApplicationFormAST>();
        while(curTok != ')') {
            form->elements.push_back( parseExpr() );
        }
        getNextToken();
        return move(form);
    }
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

void initParser()
{
    getNextToken();
}
