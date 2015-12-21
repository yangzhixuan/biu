#include "codegen.h"
#include "parser.h"
#include "typechecker.h"
#include <memory>
#include <set>

using namespace llvm;

//----------------- Globals -----------------------//
std::unique_ptr<Module> theModule;
IRBuilder<> Builder(llvm::getGlobalContext());
//----------------- Globals -----------------------//

void initCodeGenerator()
{
    theModule = llvm::make_unique<Module>("biu", llvm::getGlobalContext());
}


//------------------ Generating Functions ---------//
Value* ASTBase::codeGen(ValueEnvironment &e) { } 

Value* FormsAST::codeGen(ValueEnvironment &e)
{
    auto mainSignature = FunctionType::get(Type::getInt32Ty(getGlobalContext()), {}, false);
    Function *F = Function::Create(mainSignature, Function::ExternalLinkage, 
            "main", theModule.get());

    // Create Basic block
    BasicBlock *bb = BasicBlock::Create(theModule->getContext(), "entry", F);
    Builder.SetInsertPoint(bb);
    for(auto &f: forms) {
        f->codeGen(e);
    }
    auto ret = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
    Builder.CreateRet(ret);
    return ret;
}

Value* NumberAST::codeGen(ValueEnvironment &e)
{
    return ConstantFP::get(theModule->getContext(), APFloat(value));
}

Value* StringAST::codeGen(ValueEnvironment &e)
{
    return ConstantDataArray::getString(theModule->getContext(), str, true);
}

Value* SymbolAST::codeGen(ValueEnvironment &e)
{
    return e[identifier];
}

Value* DefineVarFormAST::codeGen(ValueEnvironment &e)
{
    Value* v = value->codeGen(e);
    e[name->identifier] = v;
    return ConstantInt::getTrue(theModule->getContext());
}

Value* ExternRawFormAST::codeGen(ValueEnvironment &e)
{
    auto g = new GlobalVariable(*theModule, varType->llvmType, false, GlobalValue::ExternalLinkage, nullptr, symbolName->str);
    e[name->identifier] = Builder.CreateLoad(g);
    return ConstantInt::getTrue(theModule->getContext());
}

Value* DefineFuncFormAST::codeGen(ValueEnvironment &e)
{
    // Create function
    // TODO: use a hashed unique name instead of the original name
    Function *F = Function::Create(flatFuncType, Function::ExternalLinkage, 
            name->identifier, theModule.get());

    int idx = 0;
    for(auto & arg : F -> args()) {
        if(idx == 0){
            arg.setName("closure");
        } else {
            arg.setName(argList[idx].first->identifier);
            idx++;
        }
    }

    // Create Basic block
    BasicBlock *bb = BasicBlock::Create(theModule->getContext(), "entry", F);
    BasicBlock *oldBB = Builder.GetInsertBlock();
    Builder.SetInsertPoint(bb);
    ValueEnvironment newEnv;

    // Binds free variables (variables in closure)
    auto ite = F->arg_begin();
    auto strV = Builder.CreateLoad(ite);
    for(auto & freeVarIter : freeVarIndex) {
        //Value *addr = Builder.CreateStructGEP(ite, freeVarIter.second);
        //Value *val = Builder.CreateLoad(addr);
        auto val = Builder.CreateExtractValue(strV, { (unsigned) freeVarIter.second });
        newEnv[freeVarIter.first] = val;
    }

    // Binds function arguments
    ite++;
    int argIndex = 0;
    while(ite != F->arg_end()) {
        newEnv[argList[argIndex].first->identifier] = ite;
        ite++;
        argIndex++;
    }

    Value* last = nullptr;
    for(auto & b : body) {
        last = b->codeGen(newEnv);
    }
    Builder.CreateRet(last);
    verifyFunction(*F);
    if(oldBB) {
        Builder.SetInsertPoint(oldBB);
    }

    // Create the closure
    auto closure_v = Builder.CreateAlloca(funType->llvmType, nullptr, name->identifier);
    auto fp_v = Builder.CreateStructGEP(closure_v, 0);
    auto env_v = Builder.CreateStructGEP(closure_v, 1);
    Builder.CreateStore(F, fp_v);

    // allocate the closure in the heap
    Value * mallocSize = ConstantExpr::getSizeOf(envType);
    mallocSize = ConstantExpr::getTruncOrBitCast( cast<Constant>(mallocSize),
            Type::getInt64Ty(getGlobalContext()));
    Instruction * var_malloc= CallInst::CreateMalloc(Builder.GetInsertBlock(),
            Type::getInt64Ty(getGlobalContext()), envType, mallocSize);
    Builder.Insert(var_malloc);
    Value *var = var_malloc;
    Builder.CreateStore(var, env_v);

    // set the closure variables
    for(auto & freeVar : freeVarIndex) {
        auto ele_v = Builder.CreateStructGEP(var, freeVar.second);
        Builder.CreateStore(e[freeVar.first], ele_v);
    }
    e[name->identifier] = Builder.CreateLoad(closure_v);
    return ConstantInt::getTrue(theModule->getContext());
}

Value* ApplicationFormAST::codeGen(ValueEnvironment &e)
{
    auto ite = elements.begin();
    auto op = (*ite)->codeGen(e);
    vector<Value*> args;

    // extract the funcion pointer from the closure
    Value *fp = Builder.CreateExtractValue(op, { 0 });

    // the first argument is the environment
    Value *env = Builder.CreateExtractValue(op, { 1 });
    args.push_back(env);

    ite++;
    while(ite != elements.end()) {
        // TODO If the argument is a function, cast its type to generic function type
        auto arg = (*ite) -> codeGen(e);
        cerr<<"Arg: "<<arg<<std::endl;
        args.push_back(arg);
        ite++;
    }

    return Builder.CreateCall(fp, args);
}

Value* IfFormAST::codeGen(ValueEnvironment &e)
{
    auto cond = condition->codeGen(e);
    Function *theFunction = Builder.GetInsertBlock()->getParent();

    BasicBlock *thenBB =
        BasicBlock::Create(getGlobalContext(), "then", theFunction);
    BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    Builder.CreateCondBr(cond, thenBB, elseBB);

    // Emit then value.
    Builder.SetInsertPoint(thenBB);

    ValueEnvironment newEnv1(e);
    auto thenV = branch_true->codeGen(newEnv1);

    Builder.CreateBr(mergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    thenBB = Builder.GetInsertBlock();

    // Emit else block.
    theFunction->getBasicBlockList().push_back(elseBB);
    Builder.SetInsertPoint(elseBB);

    ValueEnvironment newEnv2(e);
    auto elseV = branch_false->codeGen(newEnv2);

    Builder.CreateBr(mergeBB);
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    elseBB = Builder.GetInsertBlock();

    // Emit merge block.
    theFunction->getBasicBlockList().push_back(mergeBB);
    Builder.SetInsertPoint(mergeBB);
    PHINode *PN =
        Builder.CreatePHI(retType->llvmType, 2, "iftmp");

    PN->addIncoming(thenV, thenBB);
    PN->addIncoming(elseV, elseBB);
    return PN;
}
