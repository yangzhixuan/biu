#include "parser.h"
#include "typechecker.h"
#include "llvm/Support/raw_ostream.h"


std::ostream& operator<<(std::ostream& out, const CheckerError& err)
{
    out << "Checker error: " << err.msg;
    return out;
}

std::ostream& operator<<(std::ostream& out, llvm::Type* t)
{
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    t->print(rso);
    out<<rso.str();
    return out;
}

//===------------ type constants --------------=====
//==================================================
auto numberType = std::make_shared<BiuType>("Number", llvm::Type::getDoubleTy(llvm::getGlobalContext()));
auto stringType = std::make_shared<BiuType>("String");
auto voidType = std::make_shared<BiuType>("Void", llvm::Type::getVoidTy(llvm::getGlobalContext()));
auto boolType = std::make_shared<BiuType>("Bool", llvm::Type::getInt1Ty(llvm::getGlobalContext()));
auto notAType = std::make_shared<BiuType>("__NAT__");
auto sizeTType = std::make_shared<BiuType>("__SIZE_T__",
        sizeof(size_t) == 4 ?
        llvm::Type::getInt32Ty(llvm::getGlobalContext())
        : llvm::Type::getInt64Ty(llvm::getGlobalContext()));

//===------------ types -----------------------=====
//==================================================
BiuType::BiuType(const std::string& id, llvm::Type* llvmType) : identifier(id), llvmType(llvmType)
{
    hashed_id = std::hash<std::string>()(id);
}

bool BiuType::operator==(const BiuType& h) const
{
    if(hashed_id == notAType->hashed_id
            || h.hashed_id == notAType->hashed_id) {
        return false;
    }
    if(hashed_id != h.hashed_id)
        return false;
    return identifier == h.identifier;
}

bool BiuType::operator!=(const BiuType& h) const
{
    return !(*this == h);
}

ArrayType::ArrayType(std::shared_ptr<BiuType> eleType) : eleType(eleType)
{
    eleSize = dataLayout->getTypeAllocSize(eleType->llvmType);

    llvmType = llvm::StructType::get(llvm::getGlobalContext(), {eleType->llvmType->getPointerTo(), sizeTType->llvmType, sizeTType->llvmType});

    std::string id;
    id = "(Array ";
    id += eleType->identifier;
    id += ")";
    identifier = id;
    hashed_id = std::hash<std::string>()(id);
}

FuncType::FuncType(const std::vector<shared_ptr<BiuType>>& args, shared_ptr<BiuType> ret, llvm::Type *lTy)
    : returnType(ret)
{
    if(lTy != nullptr) {
        llvmType = lTy;
    } else {
        std::vector<llvm::Type*> argT;
        // the first argument is always the environment
        argT.push_back(llvm::Type::getInt8Ty(llvm::getGlobalContext())->getPointerTo());
        for(const auto & e : args) {
            argT.push_back(e->llvmType);
        }
        llvmType = llvm::StructType::get(llvm::getGlobalContext(),
                                            { llvm::FunctionType::get(ret->llvmType, argT, false)->getPointerTo(),
                                            llvm::Type::getInt8Ty(llvm::getGlobalContext())->getPointerTo() }, false);
    }

    std::string id;
    id = "(";
    for(const auto& a : args) {
        if(id != "(") {
            id += ", ";
        }
        id += a->identifier;
        argTypes.push_back(a);
    }
    id += ")";
    id += " -> ";
    id += "(" + ret->identifier + ")";

    identifier = id;
    hashed_id = std::hash<std::string>()(id);
}

//------------------ ScanFreeVariables ------------//
//==================================================

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

void MakeArrayAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    numEleExpr->scanFreeVars(s, binded);
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

void GetIndexAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    array->scanFreeVars(s, binded);
    index->scanFreeVars(s, binded);
}

void SetIndexAST::scanFreeVars(std::set<pair<string,shared_ptr<BiuType>>>& s, std::set<string>& binded)
{
    array->scanFreeVars(s, binded);
    index->scanFreeVars(s, binded);
    element->scanFreeVars(s, binded);
}

//===------------ type checker ---------------======
//==================================================

shared_ptr<BiuType> ExprAST::parseType()
{
    throw(CheckerError("invalid type"));
    return nullptr;
}

shared_ptr<BiuType> SymbolAST::parseType()
{
    if(identifier == "Bool"){
        return boolType;
    }

    if(identifier == "Number") {
        return numberType;
    }
    throw(CheckerError("invalid type: " + identifier));
    return nullptr;
}

shared_ptr<BiuType> ApplicationFormAST::parseType()
{
    if(elements.size() > 1) {
        if(typeid(*elements[0]) == typeid(SymbolAST)){
            SymbolAST *ptr = dynamic_cast<SymbolAST*>(elements[0].get());
            if(ptr->identifier == "->" && elements.size() >= 2) {
                std::vector<shared_ptr<BiuType>> argTypes;
                shared_ptr<BiuType> retType;
                for(int i = 1; i < elements.size() - 1; i++) {
                    argTypes.push_back( elements[i]->parseType() );
                }
                retType = elements[elements.size() - 1]->parseType();
                return std::make_shared<FuncType>(argTypes, retType);
            } else if(ptr->identifier == "Array" && elements.size() == 2) {
                auto eleType = elements[1]->parseType();
                return std::make_shared<ArrayType>(eleType);
            }
        }
    }
    throw(CheckerError("invalid type"));
    return nullptr;
}


shared_ptr<BiuType> ASTBase::checkType(TypeEnvironment &e)
{
    return nullptr;
}

shared_ptr<BiuType> DefineVarFormAST::checkType(TypeEnvironment &e)
{
    // TODO add define-var type annotating
    varType = e[name->identifier] = value->checkType(e);
    cerr<<"Biu type of "<<name->identifier<<" : "<<e[name->identifier]->identifier<<std::endl;
    cerr<<"IR type of "<<name->identifier<<" : "<<e[name->identifier]->llvmType<<std::endl<<std::endl;
    return voidType;
}


// This function need not only check the function type in biu's type system,
// it also needs to build the function type in the IR.
shared_ptr<BiuType> DefineFuncFormAST::checkType(TypeEnvironment &e)
{
    // Check in biu
    shared_ptr<BiuType> retType;
    std::vector<shared_ptr<BiuType>> argTypes;
    TypeEnvironment newEnv(e);

    if(type != nullptr) {
        retType = type->parseType();
    }

    for(const auto & a : argList) {
        shared_ptr<BiuType> t = a.second->parseType();
        newEnv[a.first->identifier] = t;
        argTypes.push_back(t);
    }

    if(type != nullptr) {
        newEnv[name->identifier] = std::make_shared<FuncType>(argTypes, retType);
    } else {
        // If DefineFuncForm doesn't specify the return type, don't allow recursion
        newEnv[name->identifier] = notAType;
    }

    shared_ptr<BiuType> lastType;
    for(const auto & exp : body) {
        lastType = exp->checkType(newEnv);
    }
    if(type != nullptr && !(*lastType == *retType)) {
        throw(CheckerError("function return type mismatch\n"));
    }

    // Build type in the IR
    retTypeIR = lastType->llvmType;

    // In order to get the type in IR, need to identify free variables in IR
    freeVars.clear();
    std::set<string> binded;
    scanFreeVars(freeVars, binded);
    if(type != nullptr) {
        // this maybe a recursive function, add the function name in the envrionment
        // TODO add this free variable in need (i.e. this function is indeed recursive)
        freeVars.insert(make_pair(name->identifier, std::make_shared<FuncType>(argTypes, retType)));
    }

    // Construct the type of the closure
    freeVarIndex.clear();
    vector<llvm::Type*> freeVarTypes;
    for(const auto & i : freeVars) {
        freeVarIndex[i.first] = freeVarIndex.size();
        freeVarTypes.push_back(i.second->llvmType);
    }
    envType = llvm::StructType::get(llvm::getGlobalContext(), freeVarTypes, false);

    // Construct the type of the function in IR
    funcArgTypeIR.clear();
    funcArgTypeIR.push_back(envType->getPointerTo());
    for(const auto & e: argList) {
        funcArgTypeIR.push_back(e.second->parseType()->llvmType);
    }
    flatFuncType = llvm::FunctionType::get(lastType->llvmType, funcArgTypeIR, false);

    funType = std::make_shared<FuncType>(argTypes, lastType);
    funType->llvmType = llvm::StructType::get(llvm::getGlobalContext(), {flatFuncType->getPointerTo(), envType->getPointerTo()}, false);

    e[name->identifier] = funType;
    cerr<<"BiuType of "<<name->identifier<<" : "<<e[name->identifier]->identifier<<std::endl;
    cerr<<"IR type of "<<name->identifier<<" : "<<funType->llvmType<<std::endl<<std::endl;

    return voidType;
}

shared_ptr<BiuType> ApplicationFormAST::checkType(TypeEnvironment &e) {
    if(elements.size() > 0) {
        auto opType = elements[0]->checkType(e);
        std::vector<shared_ptr<BiuType>> argTypes;
        for(int i = 1; i < elements.size(); i++) {
            argTypes.push_back(elements[i]->checkType(e));
        }
        if(auto pFT = dynamic_cast<FuncType*>(opType.get())) {
            if(pFT->argTypes.size() != argTypes.size()) {
                throw(CheckerError("ApplicationForm number of arguments mismatch"));
                return nullptr;
            }
            for(int i = 0; i < argTypes.size(); i++) {
                if(*pFT->argTypes[i] != *argTypes[i]) {
                    throw(CheckerError("ApplicationForm mismatch of type of argument, expected: "
                                       + pFT->argTypes[i]->identifier + ", got: " + argTypes[i]->identifier));
                    return nullptr;
                }
            }
            return pFT->returnType;
        } else {
            throw(CheckerError("ApplicationForm the operator must be a function, got: " + opType->identifier));
        }
    }
    throw(CheckerError("ApplicationForm should have at least one element"));
    return nullptr;
}

shared_ptr<BiuType> StringAST::checkType(TypeEnvironment &e) {
    return stringType;
}

shared_ptr<BiuType> NumberAST::checkType(TypeEnvironment &e) {
    return numberType;
}

shared_ptr<BiuType> SymbolAST::checkType(TypeEnvironment &e) {
    if(e.find(identifier) != e.end()) {
        return symbolType = e[identifier];
    }
    throw(CheckerError("Cannot resolve type of " + identifier));
    return nullptr;
}

shared_ptr<BiuType> MakeArrayAST::checkType(TypeEnvironment &e) {
    eleType = eleTypeExpr->parseType();
    if(numEleExpr->checkType(e) != numberType) {
        throw(CheckerError("make-array: array length should be a number"));
        return nullptr;
    }
    arrType = std::make_shared<ArrayType>(eleType);
    return arrType;
}


shared_ptr<BiuType> SetIndexAST::checkType(TypeEnvironment &e) {
    auto arrType = array->checkType(e);
    auto idxType = index->checkType(e);
    auto eleType = element->checkType(e);
    if(auto pAT = dynamic_cast<ArrayType*>( arrType.get())) {
        if(idxType != numberType) {
            CheckerError("set: second argument should be a number");
            return nullptr;
        }
        if(*eleType != *pAT->eleType) {
            CheckerError("set: array type and element type mismatch: " + eleType->identifier 
                    + " and " + pAT->identifier);
            return nullptr;
        }
        this->arrType = std::make_shared<ArrayType>(pAT->eleType);
        return voidType;
    } else {
        CheckerError("set: first argument should be an array");
        return nullptr;
    }
}

shared_ptr<BiuType> GetIndexAST::checkType(TypeEnvironment &e) {
    auto arrType = array->checkType(e);
    auto idxType = index->checkType(e);
    if(auto pAT = dynamic_cast<ArrayType*>( arrType.get())) {
        if(idxType != numberType) {
            ParserError("get: second argument should be a number");
            return nullptr;
        }
        this->arrType = std::make_shared<ArrayType>(pAT->eleType);
        return pAT->eleType;
    } else {
        ParserError("get: first argument should be an array");
        return nullptr;
    }
}

shared_ptr<BiuType> ExternRawFormAST::checkType(TypeEnvironment &e) {
    auto t = type->parseType();
    varType = e[name->identifier] = t;
    cerr<<"Biu type of "<<name->identifier<<" : "<<varType->identifier<<std::endl;
    cerr<<"IR type of "<<name->identifier<<" : "<<varType->llvmType<<std::endl<<std::endl;;
    return voidType;
}

shared_ptr<BiuType> IfFormAST::checkType(TypeEnvironment &e) {
    auto t = condition->checkType(e);
    if(*t != *boolType) {
        throw(CheckerError("IfForm condition must be bool, got " + t->identifier));
        return nullptr;
    }
    TypeEnvironment newEnv1(e);
    TypeEnvironment newEnv2(e);
    auto b1 = branch_true->checkType(newEnv1);
    auto b2 = branch_false->checkType(newEnv2);
    if(*b1 != *b2) {
        throw(CheckerError("IfForm type of branches mismatch, got " + b1->identifier + " and " + b2->identifier));
        return nullptr;
    }
    return retType = b1;
}

shared_ptr<BiuType> FormsAST::checkType(TypeEnvironment &e)
{
    for(const auto & f : forms) {
        f->checkType(e);
    }
    return voidType;
}
