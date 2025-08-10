#pragma once

#include <iostream>

#include "tokenizer.h"
#include "jit.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include <cstdint>
#include <map>
#include <ranges>
#include <string>

#define DEBUG 0

#if DEBUG
template <class... Args>
void debug(int layer, Args&&... args)
{
    std::cerr << std::string(layer * 4, ' ');
    ((std::cerr << args << ' '), ...) << std::endl;
}
#else
template <class... Args>
void debug(int layer, Args&&... args)
{}
#endif

namespace Token {

int getTokenPrecedence(const std::string& token) // NOLINT
{
    if (binopPrecedence.contains(token)) {
        return binopPrecedence.at(token);
    }
    return -1;
}

} // namespace Token

namespace AST {

class PrototypeAST;

class IRManager;

namespace {
static IRManager *_this = nullptr;

// jit
std::unique_ptr<llvm::orc::ShitJIT> __jit;

// Values
std::map<std::string, llvm::Value*> __values;
std::map<std::string, std::unique_ptr<PrototypeAST>> __functionProtos;

// Error
llvm::ExitOnError __exitOnErr;
} // namespace

class IRManager {
public:
    using Context     = llvm::LLVMContext;
    using Builder     = llvm::IRBuilder<>;
    using Module      = llvm::Module;

    static IRManager *get()
    {
        if (_this == nullptr) {
            reinit();
        }
        return _this;
    }

    static void reinit()
    {
        _this = new IRManager();
    }

    static Context *getCtx()
    {
        return IRManager::get()->context_.get();
    }

    static Builder *getBuilder()
    {
        return IRManager::get()->builder_.get();
    }

    static Module *getModule()
    {
        return IRManager::get()->module_.get();
    }

    static llvm::orc::ShitJIT *getJIT()
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

    static std::map<std::string, llvm::Value*> &getValues()
    {
        return __values;
    }

    static std::map<std::string, std::unique_ptr<PrototypeAST>> &getFunctionProtos()
    {
        return __functionProtos;
    }

    static llvm::Function *getFunction(std::string name);

    template <class T>
    static auto onErr(T arg)
    {
        return __exitOnErr(std::move(arg));
    }

// private:
    IRManager()
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

    static std::unique_ptr<llvm::orc::ShitJIT> createJIT()
    {
        auto jit = llvm::orc::ShitJIT::Create();
        if (auto err = jit.takeError()) {
            llvm::errs() << "Cannot create a JIT " << toString(std::move(err)) << "\n";
            return nullptr;
        }
        return std::unique_ptr<llvm::orc::ShitJIT>(jit->release());
    }

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
};


// AST types

class ExpressionAST {
public:

    virtual llvm::Value *codeGen() const = 0;
    virtual void debugPrint(int layer = 0) const = 0;

    virtual ~ExpressionAST() = default;
};

// Loggers

std::unique_ptr<ExpressionAST> LogError(const char *Str) // NOLINT
{
    fprintf(stderr, "Found shit: %s\n", Str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str) // NOLINT
{
    LogError(str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str) // NOLINT
{
    LogError(str);
    return nullptr;
}

// Values

class ValueAST : public ExpressionAST {
public:
    ValueAST(int64_t value)
        : value_(value)
    { }

    llvm::Value *codeGen() const override
    {
        return llvm::ConstantInt::get(
                *IRManager::getCtx(),
                llvm::APInt(64, value_));
    }

    void debugPrint(int layer = 0) const override
    {
#if DEBUG
        ++layer;
        debug(layer, "Value: ", value_);
#endif
    }

private:
    int64_t value_;
};

// using BoolAST  = ValueAST<bool, llvm::APInt>;
// using IntAST   = ValueAST<int64_t, llvm::APInt>;
// using FloatAST = ValueAST<double, llvm::APFloat>;

// class StringAST : public ExpressionAST {
//     StringAST(std::string value)
//         : value_(std::move(value))
//     { }
//
//     llvm::Value *codeGen() const override
//     {
//         return ASTMeta::getBuilder()->CreateGlobalString(value_, "asd");
//     }
//
// private:
//     std::string value_;
// };

// Variables

// llvm::Type *castType(Token::Token token)
// {
//     switch (token) {
//     case Token::BOOL:
//         return llvm::Type::getInt8Ty(*ASTMeta::getCtx());
//     case Token::INT:
//         return llvm::Type::getInt64Ty(*ASTMeta::getCtx());
//     case Token::FLOAT:
//         return llvm::Type::getDoubleTy(*ASTMeta::getCtx());
//     default:
//         return llvm::Type::getInt64Ty(*ASTMeta::getCtx());
//     }
// }

class VarAST : public ExpressionAST {
public:
    VarAST(std::string name)
        : name_(std::move(name))
    { }

    llvm::Value *codeGen() const override
    {
        llvm::Value *value = IRManager::getValues()[name_];
        return value
            ? value
            : IRManager::getBuilder()->CreateAlloca(
                llvm::Type::getInt64Ty(*IRManager::getCtx()),
                nullptr,
                name_);
    }

    void debugPrint(int layer = 0) const override
    {
#if DEBUG
        ++layer;
        debug(layer, "Var: ", name_);
#endif
    }

private:
    std::string name_;
};

// Operators

class BinaryExprAST : public ExpressionAST {
public:
    BinaryExprAST(
            std::string op,
            std::unique_ptr<ExpressionAST> lhs,
            std::unique_ptr<ExpressionAST> rhs)
        : op_(op),
          lhs_(std::move(lhs)),
          rhs_(std::move(rhs))
    { }

    llvm::Value *codeGen() const override
    {
        llvm::Value *lhs = lhs_->codeGen();
        llvm::Value *rhs = rhs_->codeGen();
        if (!lhs || !rhs) {
            return nullptr;
        }

        if (op_ == "+") {
            return IRManager::getBuilder()->CreateAdd(lhs, rhs, "addtmp");
        }
        if (op_ == "-") {
            return IRManager::getBuilder()->CreateSub(lhs, rhs, "rhssubtmp");
        }
        if (op_ == "*") {
            return IRManager::getBuilder()->CreateMul(lhs, rhs, "rhsmultmp");
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
            return IRManager::getBuilder()->CreateStore(rhs, lhs);
        }
        std::string msg = "invalid binary operator ";
        msg += op_;
        msg += ("for " + lhs->getName() + " | " + rhs->getName()).str();
        return LogErrorV(msg.c_str());
    }

    void debugPrint(int layer = 0) const override
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

    std::string getOp() const
    {
        return op_;
    }

private:
    llvm::Value *less(llvm::Value *lhs, llvm::Value *rhs) const
    {
        lhs = IRManager::getBuilder()->CreateFCmpULT(lhs, rhs, "cmptmp");
        return IRManager::getBuilder()->CreateUIToFP(
                lhs,
                llvm::Type::getInt64Ty(*IRManager::getCtx()),
                "booltmp");
    }

    std::string op_;
    std::unique_ptr<ExpressionAST> lhs_, rhs_;
};

// Functions

class CallExprAST : public ExpressionAST {
public:
    CallExprAST(
            std::string callee,
            std::vector<std::unique_ptr<ExpressionAST>> args)
        : callee_(std::move(callee)),
          args_(std::move(args))
    { }

    llvm::Value *codeGen() const override
    {
        llvm::Function *calleeF = IRManager::getFunction(callee_);
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

        return IRManager::getBuilder()->CreateCall(
                calleeF,
                argsValues,
                "calltmp");
    }

    void debugPrint(int layer = 0) const override
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

private:
    std::string callee_;
    std::vector<std::unique_ptr<ExpressionAST>> args_;
};

class PrototypeAST {
public:
    PrototypeAST(
            std::string name,
            std::vector<std::string> args)
        : name_(std::move(name)),
          args_(std::move(args))
    { }

    const std::string &getName() const
    {
        return name_;
    }

    llvm::Function *codeGen() const
    {
        std::vector<llvm::Type *> argsTypes(args_.size(), getType());

        llvm::Function *func =
                llvm::Function::Create(
                        llvm::FunctionType::get(getType(), argsTypes, false),
                        llvm::Function::ExternalLinkage,
                        name_,
                        IRManager::getModule());

        for (auto arg : std::views::zip(func->args(), args_)) {
            std::get<0>(arg).setName(std::get<1>(arg));
        }

        return func;
    }

    void debugPrint(int layer = 0) const
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

private:
    static llvm::IntegerType *getType()
    {
        return llvm::Type::getInt64Ty(*IRManager::getCtx());
    }

    std::string name_;
    std::vector<std::string> args_;
};

class FunctionAST {
public:
    FunctionAST(
            std::unique_ptr<PrototypeAST> proto,
            std::unique_ptr<ExpressionAST> body)
        : proto_(std::move(proto)),
          body_(std::move(body))
    { }

    llvm::Function *codeGen()
    {
        auto &protoPtr = *proto_;
        IRManager::getFunctionProtos()[proto_->getName()] = std::move(proto_);
        llvm::Function *func = IRManager::getFunction(protoPtr.getName());
        if (!func) {
            return nullptr;
        }

        IRManager::getBuilder()->SetInsertPoint(
                llvm::BasicBlock::Create(
                        *IRManager::getCtx(),
                        "entry",
                        func));

        IRManager::getValues().clear();
        for (auto &arg : func->args()) {
            IRManager::getValues()[std::string(arg.getName())] = &arg;
        }

        if (llvm::Value *retVal = body_->codeGen()) {
            IRManager::getBuilder()->CreateRet(retVal);
            llvm::verifyFunction(*func);
            IRManager::get()->fpm_->run(*func, *IRManager::get()->fam_);
            return func;
        }

        func->eraseFromParent();
        return nullptr;
    }

    void debugPrint(int layer = 0) const
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

private:
    std::unique_ptr<PrototypeAST> proto_;
    std::unique_ptr<ExpressionAST> body_;
};


llvm::Function *IRManager::getFunction(std::string name) // NOLINT
{
    if (auto *func = IRManager::get()->module_->getFunction(name)) {
        return func;
    }

    auto funcIt = __functionProtos.find(name);
    if (funcIt != __functionProtos.end()) {
        return funcIt->second->codeGen();
    }

    return nullptr;
}


} // namespace AST

namespace Parser {

class Parser {
public:
    Parser()
        : token_(Token::END, nullptr)
    { }

    // number
    std::unique_ptr<AST::ExpressionAST> parseValue()
    {
        auto result = std::make_unique<AST::ValueAST>(getTokenData<int64_t>());
        getToken();
        return result;
    }

    // parentheses
    // seq('(', optional(@expression), ')')
    std::unique_ptr<AST::ExpressionAST> parseParentheses()
    {
        getToken(); // eject '('

        auto expr = parseExpression();
        if (!expr) {
            return nullptr;
        }

        if (getTokenName() != ")") {
            return AST::LogError("Expected ')' in expression");
        }
        getToken(); // eject ')'

        return expr;
    }

    // identifier | call
    // @identifier
    // @call: seq(
    //          @identifier
    //          '(',
    //          repeat(
    //              seq(@identifier, optional(','))),
    //          ')')
    std::unique_ptr<AST::ExpressionAST> parseIdentifier()
    {
        auto name = getTokenName();
        getToken();
        if (getTokenName() != "(") { // not a 'call'
            return std::make_unique<AST::VarAST>(name);
        }
        getToken(); // eject '('

        // 'call'
        std::vector<std::unique_ptr<AST::ExpressionAST>> args;
        if (getTokenName() != ")") {
            while (true) {
                auto arg = parseExpression();
                if (arg) {
                    args.push_back(std::move(arg));
                }
                else {
                    return nullptr;
                }

                if (getTokenName() == ")")
                    break;

                if (getTokenName() != ",")
                    return AST::LogError("Expected ')' or ',' in argument list");

                getToken();
            }
        }

        getToken(); // eject ')'
        return std::make_unique<AST::CallExprAST>(name, std::move(args));
    }

    // primary
    // choice(@identifier,
    //        @call,
    //        @number,
    //        @parentheses)
    std::unique_ptr<AST::ExpressionAST> parsePrimary()
    {
        if (getTokenName() == "(") {
            return parseParentheses();
        }
        if (token_.first == Token::IDENT) {
            return parseIdentifier();
        }
        if (token_.first == Token::INT) {
            return parseValue();
        }
        return AST::LogError("Unknown token when expecting an expression");
    }

    // expression
    // seq(@expression, @bin_op, @expression)
    std::unique_ptr<AST::ExpressionAST> parseExpression()
    {
        auto lhs = parsePrimary();
        if (!lhs) {
            return nullptr;
        }
        return parseRhsBinOp(0, std::move(lhs));
    }

    std::unique_ptr<AST::ExpressionAST> parseRhsBinOp(
            int exprPrec,
            std::unique_ptr<AST::ExpressionAST> lhs)
    {
        // parse rhs while operators precedence lower than current op
        while (true) {
            int prec = Token::getTokenPrecedence(getTokenName());

            if (prec < exprPrec)
                return lhs;

            std::string op = getTokenName();
            getToken();

            auto rhs = parsePrimary();
            if (!rhs) {
                return nullptr;
            }

            int nextPrecedence = Token::getTokenPrecedence(getTokenName());
            if (exprPrec < nextPrecedence) {
                rhs = parseRhsBinOp(exprPrec + 1, std::move(rhs));
                if (!rhs) {
                    return nullptr;
                }
            }

            // merge lhs(op)rhs
            lhs = std::make_unique<AST::BinaryExprAST>(op, std::move(lhs), std::move(rhs));
        }
        return lhs;
    }

    // @prototype
    // seq(
    //     @identifier
    //     '(',
    //     repeat(
    //         seq(@identifier, optional(','))),
    //     ')')
    std::unique_ptr<AST::PrototypeAST> parsePrototype()
    {
        std::string name = getTokenName();
        if (name.empty()) {
            return AST::LogErrorP("Expected function name in prototype");
        }

        getToken(); // eject name
        if (getTokenName() != "(") {
            return AST::LogErrorP("Expected '(' in prototype");
        }
        getToken(); // eject '('

        std::vector<std::string> args;
        while (getTokenName() != ")") {
            args.push_back(getTokenName());

            // eject ','
            getToken();
            auto token = getTokenName();
            if (token == ")") {
                break;
            }
            if (token != ",") {
                std::string msg = "Expected ',' between args in args list prototype, found: " + token;
                return AST::LogErrorP(msg.c_str());
            }
            getToken();
        }

        if (getTokenName() != ")") {
            return AST::LogErrorP("Expected ')' in prototype");
        }

        getToken(); // eject ')'
        return std::make_unique<AST::PrototypeAST>(std::move(name), std::move(args));
    }

    // function_defenition
    // seq(
    //      defenition: @prototype,
    //      body:       @expression)
    std::unique_ptr<AST::FunctionAST> parseDefinition()
    {
        getToken(); // eject 'fun'

        auto proto = parsePrototype();
        if (!proto) {
            return nullptr;
        }
        auto body = parseExpression();
        if (!body) {
            return nullptr;
        }

        return std::make_unique<AST::FunctionAST>(std::move(proto), std::move(body));
    }

    // extern
    // seq(
    //      'extern',
    //      @prototype)
    std::unique_ptr<AST::PrototypeAST> parseExtern()
    {
        getToken(); // eject 'extern'
        return parsePrototype();
    }

    // @expression
    std::unique_ptr<AST::FunctionAST> parseTopLevelExpr()
    {
        auto expr = parseExpression();
        // if (isAssigment(expr)) {
        //     debug(0, "ASSIGMENT");
        //     return expr;
        // }
        if (!expr) {
            return nullptr;
        }
        auto proto = std::make_unique<AST::PrototypeAST>("__anon_expr", std::vector<std::string>());
        return std::make_unique<AST::FunctionAST>(std::move(proto), std::move(expr));
    }

    // Handlers
    void HandleDefinition()
    {
        auto funcAST = parseDefinition();
        funcAST->debugPrint();
        if (funcAST) {
            auto *funcIR = funcAST->codeGen();
            if (funcIR) {
                fprintf(stderr, "Read function definition:\n");
                funcIR->print(llvm::errs());
                fprintf(stderr, "\n");

                AST::IRManager::onErr(
                    AST::IRManager::getJIT()->addModule(
                        llvm::orc::ThreadSafeModule(
                            std::move(AST::IRManager::get()->module_),
                            std::move(AST::IRManager::get()->context_))));

                AST::IRManager::reinit();
            }
        }
        else {
            getToken();
        }
    }

    void HandleExtern()
    {
        auto protoAST = parseExtern();
        protoAST->debugPrint();
        if (protoAST) {
            auto *funcIR = protoAST->codeGen();
            if (funcIR) {
                fprintf(stderr, "Read extern:\n");
                funcIR->print(llvm::errs());
                fprintf(stderr, "\n");
                AST::IRManager::getFunctionProtos()[protoAST->getName()] = std::move(protoAST);
            }
        }
        else {
            getToken();
        }
    }

    void HandleTopLevelExpression()
    {
        auto funcAST = parseTopLevelExpr();
        funcAST->debugPrint();
        if (funcAST) {
            if (funcAST->codeGen()) {
                auto retType = AST::IRManager::getJIT()->getMainJITDylib().createResourceTracker();

                auto tsm = llvm::orc::ThreadSafeModule(
                        std::move(AST::IRManager::get()->module_),
                        std::move(AST::IRManager::get()->context_));

                AST::IRManager::onErr(AST::IRManager::getJIT()->addModule(std::move(tsm), retType));
                AST::IRManager::reinit();

                auto exprSymbol = AST::IRManager::onErr(AST::IRManager::getJIT()->lookup("__anon_expr"));

                int64_t (*intPtr)() = exprSymbol.toPtr<int64_t (*)()>();
                fprintf(stderr, "Evaluated to %ld\n", intPtr());

                AST::IRManager::onErr(retType->remove());
            }
        }
        else {
            getToken();
        }
    }

    // top
    // choice(
    //      @definition,
    //      @external,
    //      @expression,
    //      ';')
    void MainLoop()
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        AST::IRManager::reinit();

        fprintf(stderr, "post> ");
        getToken();

        while (true) {
            if (token_.first == Token::END) {
                fprintf(stderr, "\n==== done ====\n");
                return;
            }
            else if (token_.first == Token::FUNC) {
                HandleDefinition();
            }
            else if (token_.first == Token::EXT) {
                HandleExtern();
            }
            else if (getTokenName() == ";") {
                getToken();
            }
            else {
                HandleTopLevelExpression();
            }
            fprintf(stderr, "post> ");
        }

        AST::IRManager::getModule()->print(llvm::errs(), nullptr);
    }
private:
    Token::TokenData getToken()
    {
        Token::freeToken(token_);
        token_ = tokenizer_.getToken();
        return token_;
    }

    template <class T>
    T getTokenData() const
    {
        return *static_cast<T*>(token_.second);
    }

    std::string getTokenName() const
    {
        if (token_.first != Token::IDENT) {
            return {};
        }
        return getTokenData<std::string>();
    }

    bool isAssigment(const std::unique_ptr<AST::ExpressionAST>& expr)
    {
        auto binExpr = dynamic_cast<AST::BinaryExprAST*>(expr.get());
        return binExpr != nullptr
            && binExpr->getOp() == "=";
    }

    Token::Tokenizer tokenizer_;
    Token::TokenData token_;
};

} // namespace Parser
