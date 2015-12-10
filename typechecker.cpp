#include "parser.h"
#include "typechecker.h"

std::ostream& operator<<(std::ostream& out, const CheckerError& err)
{
    out << "Checker error: " << err.msg;
    return out;
}

//===------------ types -----------------------=====
//==================================================
Type::Type(const std::string& id) : identifier(id)
{
    hashed_id = std::hash<std::string>()(id);
}

bool Type::operator==(const Type& h) const
{
    if(hashed_id != h.hashed_id)
        return false;
    return identifier == h.identifier;
}

bool Type::operator!=(const Type& h) const
{
    return !(*this == h);
}

FuncType::FuncType(const std::vector<shared_ptr<Type>>& args, shared_ptr<Type> ret)
    : returnType(ret)
{
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


//===------------ type checker ---------------======
//==================================================

shared_ptr<Type> ExprAST::parseType()
{
    throw(CheckerError("invalid type"));
    return nullptr;
}

shared_ptr<Type> SymbolAST::parseType()
{
    if(identifier == "Bool"){
        return std::make_shared<Type>("Bool");
    }

    if(identifier == "Int") {
        return std::make_shared<Type>("Int");
    }
    throw(CheckerError("invalid type: " + identifier));
    return nullptr;
}

shared_ptr<Type> ApplicationFormAST::parseType()
{
    if(elements.size() > 1) {
        if(typeid(*elements[0]) == typeid(SymbolAST)){
            SymbolAST *ptr = dynamic_cast<SymbolAST*>(elements[0].get());
            if(ptr->identifier == "->" && elements.size() >= 2) {
                std::vector<shared_ptr<Type>> argTypes;
                shared_ptr<Type> retType;
                for(int i = 1; i < elements.size() - 1; i++) {
                    argTypes.push_back( elements[i]->parseType() );
                }
                retType = elements[elements.size() - 1]->parseType();
                return std::make_shared<FuncType>(argTypes, retType);
            }
        }
    }
    throw(CheckerError("invalid type"));
    return nullptr;
}


shared_ptr<Type> ASTBase::checkType(Environment &e)
{
    return nullptr;
}

shared_ptr<Type> DefineVarFormAST::checkType(Environment &e)
{
    // TODO add define-var type annotating
    e[name->identifier] = value->checkType(e);
    cerr<<"Type of "<<name->identifier<<" : "<<e[name->identifier]->identifier<<std::endl;
    return std::make_shared<Type>("Void");
}

shared_ptr<Type> DefineFuncFormAST::checkType(Environment &e)
{
    shared_ptr<Type> retType;
    std::vector<shared_ptr<Type>> argTypes;
    Environment newEnv(e);

    if(type != nullptr) {
        retType = type->parseType();
    }

    for(const auto & a : argList) {
        shared_ptr<Type> t = a.second->parseType();
        newEnv[a.first->identifier] = t;
        argTypes.push_back(t);
    }

    if(type != nullptr) {
        newEnv[name->identifier] = std::make_shared<FuncType>(argTypes, retType);
    }

    shared_ptr<Type> lastType;
    for(const auto & exp : body) {
        lastType = exp->checkType(newEnv);
    }
    if(type != nullptr && !(*lastType == *retType)) {
        throw(CheckerError("function return type mismatch\n"));
    }
    auto t = std::make_shared<FuncType>(argTypes, lastType);
    e[name->identifier] = t;
    cerr<<"Type of "<<name->identifier<<" : "<<e[name->identifier]->identifier<<std::endl;
    return std::make_shared<Type>("Void");
}

shared_ptr<Type> ApplicationFormAST::checkType(Environment &e) {
    if(elements.size() > 0) {
        auto opType = elements[0]->checkType(e);
        std::vector<shared_ptr<Type>> argTypes;
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

shared_ptr<Type> StringAST::checkType(Environment &e) {
    return std::make_shared<Type>("String");
}

shared_ptr<Type> NumberAST::checkType(Environment &e) {
    return std::make_shared<Type>("Int");
}

shared_ptr<Type> SymbolAST::checkType(Environment &e) {
    if(e.find(identifier) != e.end()) {
        return e[identifier];
    }
    throw(CheckerError("Cannot resolve type of " + identifier));
    return nullptr;
}

shared_ptr<Type> ExternFormAST::checkType(Environment &e) {
    auto t = type->parseType();
    e[name->identifier] = t;
    return std::make_shared<Type>("Void");
}

shared_ptr<Type> FormsAST::checkType(Environment &e)
{
    for(const auto & f : forms) {
        f->checkType(e);
    }
    return std::make_shared<Type>("Int");
}
