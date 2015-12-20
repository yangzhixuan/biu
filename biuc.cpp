#include "parser.h"
#include "typechecker.h"
int main(int argc, const char *argv[])
{
    try{
        initParser();
        auto ast = parseForms();

        TypeEnvironment env;
        ast->checkType(env);
        std::cout<<"finished typechecking"<<std::endl;

        initCodeGenerator();

         ValueEnvironment vEnv;
         ast->codeGen(vEnv);

         theModule->dump();
    }catch(const LexerError &e){
        cerr<<e<<std::endl;
    }catch(const ParserError &e){
        cerr<<e<<std::endl;
    }catch(const CheckerError &e) {
        cerr<<e<<std::endl;
    }
    return 0;
}
