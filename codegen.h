#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

using llvm::Value;
using llvm::Module;
using llvm::IRBuilder;

typedef std::map<std::string, Value*> ValueEnvironment;

void initCodeGenerator();
extern std::unique_ptr<Module> theModule;
#endif
