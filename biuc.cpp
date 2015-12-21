#include "parser.h"
#include "typechecker.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Bitcode/ReaderWriter.h"

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

        // write IR to file
        //theModule->dump();
        std::error_code EC;
        llvm::raw_fd_ostream out("tmp.ll", EC, llvm::sys::fs::F_None);
        theModule->print(out, nullptr);

    }catch(const LexerError &e){
        cerr<<e<<std::endl;
    }catch(const ParserError &e){
        cerr<<e<<std::endl;
    }catch(const CheckerError &e) {
        cerr<<e<<std::endl;
    }
    return 0;
}
