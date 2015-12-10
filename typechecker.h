#ifndef __TYPECHECKER_H
#define __TYPECHECKER_H

#include <map>
#include <unordered_map>
#include <string>
#include "common.h"

class Type {
    public:
        Type(const std::string& identifier);

        std::string identifier;
        int hashed_id;
        Type() = default;
};

class FuncType : public Type {
    public:
        std::vector<Type> argTypes;
        Type returnType;

        FuncType(const std::vector<Type>& args, const Type& ret);
};

class CheckerError: public Error {
    public:
        CheckerError(std::string str) : Error(str) {}
};

bool operator==(const Type& t, const Type& h);

extern Type boolType, intType;
typedef std::map<std::string, Type> Enviroment;
#endif
