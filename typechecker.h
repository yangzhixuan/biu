#ifndef __TYPECHECKER_H
#define __TYPECHECKER_H

#include <map>
#include <unordered_map>
#include <string>
#include "common.h"

class Type {
    public:
        Type(const std::string& identifier);
        bool operator==(const Type &h) const;
        bool operator!=(const Type &h) const;

        std::string identifier;
        int hashed_id;
        Type() = default;
        virtual ~Type() = default;
};

class FuncType : public Type {
    public:
        std::vector<std::shared_ptr<Type>> argTypes;
        std::shared_ptr<Type> returnType;

        FuncType(const std::vector<std::shared_ptr<Type>>& args, std::shared_ptr<Type> ret);
};

class CheckerError: public Error {
    public:
        CheckerError(std::string str) : Error(str) {}
};


std::ostream& operator<<(std::ostream& out, const CheckerError& err);
typedef std::map<std::string, std::shared_ptr<Type>> Environment;
#endif
