#include "codegen.h"
#include "parser.h"
#include "typechecker.h"
#include <memory>
#include <set>
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

//----------------- Globals -----------------------//
std::unique_ptr<Module> theModule;
std::unique_ptr<DataLayout> dataLayout;
IRBuilder<> Builder(llvm::getGlobalContext());
//----------------- Globals -----------------------//

void initCodeGenerator()
{
    theModule = llvm::make_unique<Module>("biu", llvm::getGlobalContext());
    dataLayout = llvm::make_unique<DataLayout>(theModule.get());
}


//------------------ Generating Functions ---------//
Value* ASTBase::codeGen(ValueEnvironment &e) { return nullptr; } 

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

Value* MakeArrayAST::codeGen(ValueEnvironment &e)
{
    // get the array allocation function
    Function *F = theModule->getFunction("__make_array_func");
    if(F == nullptr) {
        FunctionType *FT =
            FunctionType::get(voidType->llvmType, {arrType->llvmType->getPointerTo(), 
                    llvm::Type::getDoubleTy(llvm::getGlobalContext()), sizeTType->llvmType}, false);
        F = Function::Create(FT, Function::ExternalLinkage, "__make_array_func", theModule.get());
    }

    auto alloc = Builder.CreateAlloca(arrType->llvmType, nullptr);

    Builder.CreateCall(F, { alloc, numEleExpr->codeGen(e), ConstantInt::get(sizeTType->llvmType, arrType->eleSize) });
    return Builder.CreateLoad(alloc);
}

Value* GetIndexAST::codeGen(ValueEnvironment &e)
{
    auto arr = array->codeGen(e);
    auto idx = index->codeGen(e);

    Function *F = theModule->getFunction("__getelement_func");

    if(F == nullptr) {
        FunctionType *FT =
            FunctionType::get(arrType->eleType->llvmType->getPointerTo(), {arrType->llvmType->getPointerTo(),
                    llvm::Type::getDoubleTy(llvm::getGlobalContext())}, false);

        F = Function::Create(FT, Function::ExternalLinkage, "__getelement_func", theModule.get());
    }

    auto alloc = Builder.CreateAlloca(arrType->llvmType, nullptr);
    Builder.CreateStore(arr, alloc);

    auto ep = Builder.CreateCall(F, { alloc, idx });
    return Builder.CreateLoad(ep);
}

Value* SetIndexAST::codeGen(ValueEnvironment &e)
{
    auto arr = array->codeGen(e);
    auto idx = index->codeGen(e);
    auto ele = element->codeGen(e);

    Function *F = theModule->getFunction("__getelement_func");

    if(F == nullptr) {
        FunctionType *FT =
            FunctionType::get(arrType->eleType->llvmType->getPointerTo(), {arrType->llvmType->getPointerTo(),
                    llvm::Type::getDoubleTy(llvm::getGlobalContext())}, false);

        F = Function::Create(FT, Function::ExternalLinkage, "__getelement_func", theModule.get());
    }
    auto alloc = Builder.CreateAlloca(arrType->llvmType, nullptr);
    Builder.CreateStore(arr, alloc);

    auto ep = Builder.CreateCall(F, { alloc, idx });
    Builder.CreateStore(ele, ep);
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
        if(freeVar.first == name->identifier) {
            // a recursive function, this free variable refers the function itself
            auto fp_v_rec = Builder.CreateStructGEP(ele_v, 0);
            auto env_v_rec = Builder.CreateStructGEP(ele_v, 1);
            env_v_rec = Builder.CreateBitCast(env_v_rec, var->getType()->getPointerTo());
            fp_v_rec = Builder.CreateBitCast(fp_v_rec, F->getType()->getPointerTo());
            Builder.CreateStore(var, env_v_rec);
            Builder.CreateStore(F, fp_v_rec);
        } else {
            Builder.CreateStore(e[freeVar.first], ele_v);
        }
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
