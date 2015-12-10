#include "parser.h"
#include "typechecker.h"

int main(int argc, const char *argv[])
{
    try{
        initParser();
        auto ast = parseForms();
        Environment env;
        ast->checkType(env);
        std::cout<<"finished"<<std::endl;
    }catch(const LexerError &e){
        cerr<<e<<std::endl;
    }catch(const ParserError &e){
        cerr<<e<<std::endl;
    }catch(const CheckerError &e) {
        cerr<<e<<std::endl;
    }
    return 0;
}
