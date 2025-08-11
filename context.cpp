#include "ast.h"
#include "context.h"


namespace Context {

namespace {
IRManager *_this = nullptr;

// jit
std::unique_ptr<llvm::orc::ShitJIT> __jit;

// Values
std::map<std::string, llvm::Value*> __values;
std::map<std::string, std::unique_ptr<AST::PrototypeAST>> __functionProtos;
} // namespace

IRManager* IRManager::get()
{
    if (_this == nullptr) {
        reinit();
    }
    return _this;
}

void IRManager::reinit()
{
    _this = new IRManager();
}

IRManager::Context* IRManager::getCtx()
{
    return get()->context_.get();
}

IRManager::Builder *IRManager::getBuilder()
{
    return get()->builder_.get();
}

IRManager::Module *IRManager::getModule()
{
    return get()->module_.get();
}

std::unique_ptr<IRManager::Context> IRManager::moveCtx()
{
    return std::move(get()->context_);
}

std::unique_ptr<IRManager::Module> IRManager::moveModule()
{
    return std::move(get()->module_);
}

llvm::orc::ShitJIT *IRManager::getJIT()
{
    if (__jit == nullptr) {
        auto jit = llvm::orc::ShitJIT::Create();
        if (auto err = jit.takeError()) {
            llvm::errs() << "Cannot create a JIT " << toString(std::move(err)) << "\n";
            return nullptr;
        }
        __jit = std::unique_ptr<llvm::orc::ShitJIT>(jit->release());
    }
    return __jit.get();
}

llvm::FunctionPassManager* IRManager::getFPM()
{
    return get()->fpm_.get();
}

llvm::FunctionAnalysisManager* IRManager::getFAM()
{
    return get()->fam_.get();
}

std::map<std::string, llvm::Value*>& IRManager::getValues()
{
    return __values;
}

std::map<std::string, std::unique_ptr<AST::PrototypeAST>>& IRManager::getFunctionProtos()
{
    return __functionProtos;
}

IRManager::IRManager()
    : context_(std::make_unique<Context>()),
      builder_(std::make_unique<Builder>(*context_)),
      module_(std::make_unique<Module>("someShitJIT", *context_)),
      fpm_(std::make_unique<llvm::FunctionPassManager>()),
      lam_(std::make_unique<llvm::LoopAnalysisManager>()),
      fam_(std::make_unique<llvm::FunctionAnalysisManager>()),
      cgam_(std::make_unique<llvm::CGSCCAnalysisManager>()),
      mam_(std::make_unique<llvm::ModuleAnalysisManager>()),
      pic_(std::make_unique<llvm::PassInstrumentationCallbacks>()),
      si_(std::make_unique<llvm::StandardInstrumentations>(
          *context_,
          /*DebugLogging*/ true))
{
   module_->setDataLayout(IRManager::getJIT()->getDataLayout());

   si_->registerCallbacks(*pic_, mam_.get());
   fpm_->addPass(llvm::InstCombinePass());
   fpm_->addPass(llvm::ReassociatePass());
   fpm_->addPass(llvm::GVNPass());
   fpm_->addPass(llvm::SimplifyCFGPass());

   llvm::PassBuilder PB;
   PB.registerModuleAnalyses(*mam_);
   PB.registerFunctionAnalyses(*fam_);
   PB.crossRegisterProxies(*lam_, *fam_, *cgam_, *mam_);
}

} // namespace Context
