#include "ast.h"

#include "llvm/IR/Verifier.h"

#include <ranges>


namespace AST {

llvm::Function *findFunction(std::string name)
{
    if (auto *func = Context::IRManager::getModule()->getFunction(name)) {
        return func;
    }

    auto funcIt = Context::IRManager::getFunctionProtos().find(name);
    if (funcIt != Context::IRManager::getFunctionProtos().end()) {
        return funcIt->second->codeGen();
    }

    return nullptr;
}

// Loggers
std::unique_ptr<ExpressionAST> LogError(const char *Str)
{
    fprintf(stderr, "Found shit: %s\n", Str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str)
{
    LogError(str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str)
{
    LogError(str);
    return nullptr;
}

// ---- AST values

// Value
ValueAST::ValueAST(int64_t value)
    : value_(value)
{ }

llvm::Value *ValueAST::codeGen() const
{
    return llvm::ConstantInt::get(
            *Context::IRManager::getCtx(),
            llvm::APInt(64, value_));
}

void ValueAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    debug(layer, "Value: ", value_);
#endif
}

// Variable
VarAST::VarAST(std::string name)
    : name_(std::move(name))
{ }

llvm::Value *VarAST::codeGen() const
{
    llvm::Value *value = Context::IRManager::getValues()[name_];
    return value
        ? value
        : Context::IRManager::getBuilder()->CreateAlloca(
            llvm::Type::getInt64Ty(*Context::IRManager::getCtx()),
            nullptr,
            name_);
}

void VarAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    debug(layer, "Var: ", name_);
#endif
}

// Binary operator
BinaryExprAST::BinaryExprAST(
        std::string op,
        std::unique_ptr<ExpressionAST> lhs,
        std::unique_ptr<ExpressionAST> rhs)
    : op_(op),
    lhs_(std::move(lhs)),
    rhs_(std::move(rhs))
{ }

llvm::Value *BinaryExprAST::codeGen() const
{
    llvm::Value *lhs = lhs_->codeGen();
    llvm::Value *rhs = rhs_->codeGen();
    if (!lhs || !rhs) {
        return nullptr;
    }

    if (op_ == "+") {
        return Context::IRManager::getBuilder()->CreateAdd(lhs, rhs, "addtmp");
    }
    if (op_ == "-") {
        return Context::IRManager::getBuilder()->CreateSub(lhs, rhs, "rhssubtmp");
    }
    if (op_ == "*") {
        return Context::IRManager::getBuilder()->CreateMul(lhs, rhs, "rhsmultmp");
    }
    if (op_ == "/") {
        return nullptr;
        // return ASTMeta::getBuilder()->CreateFP(lhs, rhs, "rhsdivtmp");
    }
    if (op_ == "<") {
        return less(lhs, rhs);
    }
    if (op_ == ">") {
        return less(rhs, lhs);
    }
    if (op_ == "=") {
        return Context::IRManager::getBuilder()->CreateStore(rhs, lhs);
    }

    // default:
    std::string msg = "invalid binary operator ";
    msg += op_;
    msg += ("for " + lhs->getName() + " | " + rhs->getName()).str();
    return LogErrorV(msg.c_str());
}

void BinaryExprAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    debug(layer, "BinaryOp:");
    lhs_->debugPrint(layer);
    debug(layer, "OP: ", op_);
    rhs_->debugPrint(layer);
    debug(layer, "End of BinaryOp");
#endif
}

std::string BinaryExprAST::getOp() const
{
    return op_;
}

llvm::Value *BinaryExprAST::less(llvm::Value *lhs, llvm::Value *rhs) const
{
    lhs = Context::IRManager::getBuilder()->CreateFCmpULT(lhs, rhs, "cmptmp");
    return Context::IRManager::getBuilder()->CreateUIToFP(
            lhs,
            llvm::Type::getInt64Ty(*Context::IRManager::getCtx()),
            "booltmp");
}

// Call
CallExprAST::CallExprAST(
        std::string callee,
        std::vector<std::unique_ptr<ExpressionAST>> args)
    : callee_(std::move(callee)),
    args_(std::move(args))
{ }

llvm::Value *CallExprAST::codeGen() const
{
    llvm::Function *calleeF = findFunction(callee_);
    if (!calleeF) {
        std::string msg = "Unknown function reference " + callee_;
        return LogErrorV(msg.c_str());
    }
    if (calleeF->arg_size() != args_.size()) {
        std::string msg = "Incorrect incorrect number of arguments passed for " + callee_;
        return LogErrorV(msg.c_str());
    }

    std::vector<llvm::Value *> argsValues;
    for (size_t i = 0; i != args_.size(); ++i) {
        argsValues.push_back(args_[i]->codeGen());
        if (!argsValues.back())
            return nullptr;
    }

    return Context::IRManager::getBuilder()->CreateCall(
            calleeF,
            argsValues,
            "calltmp");
}

void CallExprAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    debug(layer, "Call: ", callee_);
    for (size_t i = 0; i < args_.size(); ++i) {
        debug(layer, "Arg: ", i);
        args_[i]->debugPrint(layer);
    }
    debug(layer, "End of Call");
#endif
}

// Prototype
PrototypeAST::PrototypeAST(
        std::string name,
        std::vector<std::string> args)
    : name_(std::move(name)),
    args_(std::move(args))
{ }

const std::string &PrototypeAST::getName() const
{
    return name_;
}

llvm::Function *PrototypeAST::codeGen() const
{
    std::vector<llvm::Type *> argsTypes(args_.size(), getType());

    llvm::Function *func =
        llvm::Function::Create(
            llvm::FunctionType::get(getType(), argsTypes, false),
            llvm::Function::ExternalLinkage,
            name_,
            Context::IRManager::getModule());

    for (auto arg : std::views::zip(func->args(), args_)) {
        std::get<0>(arg).setName(std::get<1>(arg));
    }

    return func;
}

void PrototypeAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    debug(layer, "Proto: ", name_);
    for (size_t i = 0; i < args_.size(); ++i) {
        debug(layer, "Arg: ", i, " - ", args_[i]);
    }
    debug(layer, "End of Proto");
#endif
}

// Function
FunctionAST::FunctionAST(
        std::unique_ptr<PrototypeAST> proto,
        std::unique_ptr<ExpressionAST> body)
    : proto_(std::move(proto)),
    body_(std::move(body))
{ }

llvm::Function *FunctionAST::codeGen()
{
    auto &protoPtr = *proto_;
    Context::IRManager::getFunctionProtos()[proto_->getName()] = std::move(proto_);
    llvm::Function *func = findFunction(protoPtr.getName());
    if (!func) {
        return nullptr;
    }

    Context::IRManager::getBuilder()->SetInsertPoint(
        llvm::BasicBlock::Create(
            *Context::IRManager::getCtx(),
            "entry",
            func));

    Context::IRManager::getValues().clear();
    for (auto &arg : func->args()) {
        Context::IRManager::getValues()[std::string(arg.getName())] = &arg;
    }

    if (llvm::Value *retVal = body_->codeGen()) {
        Context::IRManager::getBuilder()->CreateRet(retVal);
        llvm::verifyFunction(*func);
        Context::IRManager::getFPM()->run(*func, *Context::IRManager::getFAM());
        return func;
    }

    func->eraseFromParent();
    return nullptr;
}

void FunctionAST::debugPrint(int layer) const
{
#if DEBUG
    ++layer;
    if (!proto_->getName().empty()) { // top layer
        debug(layer, "Func: ", proto_->getName());
        proto_->debugPrint(layer);
    }
    body_->debugPrint(layer);
    if (!proto_->getName().empty()) {
        debug(layer, "End of Func");
    }
#endif
}

} // namespace AST
