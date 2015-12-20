#ifndef __TYPECHECKER_H
#define __TYPECHECKER_H

#include <map>
#include <unordered_map>
#include <string>
#include "common.h"
#include "llvm/IR/Type.h"

class BiuType {
    public:
        BiuType(const std::string& identifier, llvm::Type *llvmType = nullptr);
        bool operator==(const BiuType &h) const;
        bool operator!=(const BiuType &h) const;

        std::string identifier;
        int hashed_id;
        BiuType() = default;
        virtual ~BiuType() = default;

        llvm::Type* llvmType;
};

class FuncType : public BiuType {
    public:
        std::vector<std::shared_ptr<BiuType>> argTypes;
        std::shared_ptr<BiuType> returnType;

        FuncType(const std::vector<std::shared_ptr<BiuType>>& args, std::shared_ptr<BiuType> ret, llvm::Type *llvmType = nullptr);
};

class CheckerError: public Error {
    public:
        CheckerError(std::string str) : Error(str) {}
};


std::ostream& operator<<(std::ostream& out, const CheckerError& err);
std::ostream& operator<<(std::ostream& out, llvm::Type* t);
typedef std::map<std::string, std::shared_ptr<BiuType>> TypeEnvironment;
#endif
