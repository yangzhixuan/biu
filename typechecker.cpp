#include "parser.h"
#include "typechecker.h"

//===------------ types -----------------------=====
//==================================================
Type::Type(const std::string& id) : identifier(id)
{
    hashed_id = std::hash<std::string>()(id);
}

bool operator==(const Type& t, const Type& h)
{
    if(t.hashed_id != h.hashed_id)
        return false;
    return t.identifier == h.identifier;
}

FuncType::FuncType(const std::vector<Type>& args, const Type& ret)
    : returnType(ret)
{
    std::string id;
    id = "(";
    for(const auto& a : args) {
        if(id != "(") {
            id += ", ";
        }
        id += a.identifier;
    }
    id += ")";
    id += " -> ";
    id += "(" + ret.identifier + ")";

    identifier = id;
    hashed_id = std::hash<std::string>()(id);
}

//===------------ type constants --------------=====
//==================================================

Type boolType("Bool"), intType("Int"), voidType("void");


//===------------ type checker ---------------======
//==================================================

Type ExprAST::parseType()
{
    throw(CheckerError("invalid type"));
    return voidType;
}

Type SymbolAST::parseType()
{
    if(identifier == "Bool")
        return boolType;
    if(identifier == "Int")
        return intType;
    throw(CheckerError("invalid type: " + identifier));
    return voidType;
}

Type ApplicationFormAST::parseType()
{
    if(elements.size() > 1) {
        if(typeid(*elements[0]) == typeid(SymbolAST)){
            SymbolAST *ptr = dynamic_cast<SymbolAST*>(elements[0].get());
            if(ptr->identifier == "->" && elements.size() >= 3) {
                std::vector<Type> argTypes;
                Type retType;
                for(int i = 2; i < elements.size() - 1; i++) {
                    argTypes.push_back( elements[i]->parseType() );
                }
                retType = elements[elements.size() - 1]->parseType();
                return FuncType(argTypes, retType);
            }
        }
    }
    throw(CheckerError("invalid type"));
    return voidType;
}


Type ASTBase::checkType(Enviroment & e) 
{ 
    return voidType;
}

Type DefineVarFormAST::checkType(Enviroment & e)
{
    // TODO add define-var type annotating
    e[name->identifier] = value->checkType(e);
    return voidType;
}

Type DefineFuncFormAST::checkType(Enviroment & e)
{
    Type retType;
    std::vector<Type> argTypes;
    if(type != nullptr) {
        retType = type->parseType();
    }

    Enviroment newEnv(e);
    for(const auto & a : argList) {
        Type t = a.second->parseType();
        newEnv[a.first->identifier] = t;
        argTypes.push_back(t);
    }

    Type lastType;
    for(const auto & exp : body) {
        lastType = exp->checkType(newEnv);
    }
    if(type != nullptr && !(lastType == retType)) {
        throw(CheckerError("function return type mismatch\n"));
    }
    return FuncType(argTypes, lastType);
}

Type FormsAST::checkType(Enviroment& e)
{
    for(const auto & f : forms) {
        f->checkType(e);
    }
    return intType;
}
