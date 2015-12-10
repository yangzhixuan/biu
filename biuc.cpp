#include "parser.h"
#include "typechecker.h"

int main(int argc, const char *argv[])
{
    std::cout<< boolType.identifier << std::endl;

    FuncType ft(vector<Type>{boolType}, intType);
    std::cout << ft.identifier << std::endl;

    FuncType ft2(vector<Type>{ft, boolType}, intType);
    std::cout<< ft2.identifier << (ft2 == ft) << ft.identifier<<std::endl;
    std::cout<< ft2.identifier << (ft2 == ft2) << ft2.identifier<<std::endl;

    try{
        initParser();
        auto ast = parseForms();
    }catch(const LexerError &e){
        cerr<<e<<std::endl;
    }catch(const ParserError &e){
        cerr<<e<<std::endl;
    }
    return 0;
}
