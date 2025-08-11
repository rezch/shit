#pragma once

#include "jit.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

namespace AST {
class PrototypeAST;
}; // namespace AST

namespace Context {

class IRManager {
public:
    using Context = llvm::LLVMContext;
    using Builder = llvm::IRBuilder<>;
    using Module  = llvm::Module;

    static IRManager *get();
    static void reinit();

    static Context *getCtx();
    static Builder *getBuilder();
    static Module *getModule();

    static std::unique_ptr<Context> moveCtx();
    static std::unique_ptr<Module> moveModule();

    static llvm::orc::ShitJIT *getJIT();
    static llvm::FunctionPassManager *getFPM();
    static llvm::FunctionAnalysisManager *getFAM();

    static std::map<std::string, llvm::Value *> &getValues();
    static std::map<std::string, std::unique_ptr<AST::PrototypeAST>> &getFunctionProtos();

    template <class T>
    static auto onErr(T arg)
    {
        return get()->__exitOnErr(std::move(arg));
    }

private:
    IRManager();

    // Building
    std::unique_ptr<Context> context_;
    std::unique_ptr<Builder> builder_;
    std::unique_ptr<Module> module_;

    // JIT
    std::unique_ptr<llvm::FunctionPassManager> fpm_;
    std::unique_ptr<llvm::LoopAnalysisManager> lam_;
    std::unique_ptr<llvm::FunctionAnalysisManager> fam_;
    std::unique_ptr<llvm::CGSCCAnalysisManager> cgam_;
    std::unique_ptr<llvm::ModuleAnalysisManager> mam_;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> pic_;
    std::unique_ptr<llvm::StandardInstrumentations> si_;

    // Error
    llvm::ExitOnError __exitOnErr;
};

} // namespace Context
