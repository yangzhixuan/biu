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

//------------------ ScanFreeVariables ------------//

void ExprAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    return;
}

void SymbolAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    if(binded.find(identifier) == binded.end()) {
        s.insert(make_pair(identifier, symbolType));
    }
    return;
}

void DefineVarFormAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    value->scanFreeVars(s, binded);
    binded.insert(name->identifier);
}

void DefineFuncFormAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    std::set<string> newBinded(binded);
    newBinded.insert(name->identifier);
    for(const auto & e : argList) {
        newBinded.insert(e.first->identifier);
    }
    for(const auto & e : body) {
        e->scanFreeVars(s, newBinded);
    }
    binded.insert(name->identifier);
}

void IfFormAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    condition->scanFreeVars(s, binded);
    auto e1 = std::set<string>(binded);
    auto e2 = std::set<string>(binded);
    branch_true->scanFreeVars(s, e1);
    branch_false->scanFreeVars(s, e2);
}

void ApplicationFormAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    for(const auto & e : elements) {
        e->scanFreeVars(s, binded);
    }
}

void ExternRawFormAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    binded.insert(name->identifier);
}

//------------------ ScanFreeVariables ------------//


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
    for(auto & freeVarIter : freeVarIndex) {
        Value *envStruct = Builder.CreateLoad(ite);
        Value *addr = Builder.CreateStructGEP(envStruct, freeVarIter.second);
        Value *val = Builder.CreateLoad(addr);
        newEnv[freeVarIter.first] = val;
    }

    // Binds function arguments
    ite++;
    while(ite != F->arg_end()) {
        newEnv[ite->getName()] = ite;
        ite++;
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
        args.push_back((*ite) -> codeGen(e));
        ite++;
    }

    return Builder.CreateCall(fp, args);
}

