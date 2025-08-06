#pragma once

#include "tokenizer.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include <cstdint>
#include <map>
#include <ranges>
#include <string>


namespace AST {

class ASTMeta {
public:
    using Context     = llvm::LLVMContext;
    using Builder     = llvm::IRBuilder<>;
    using Module      = llvm::Module;
    using NamedValues = std::map<std::string, llvm::Value *>;

    static ASTMeta *get()
    {
        static ASTMeta *_this = nullptr;
        if (_this == nullptr) {
            _this = new ASTMeta();
        }
        return _this;
    }

    static Context *getCtx()
    {
        return ASTMeta::get()->context_.get();
    }

    static Builder *getBuilder()
    {
        return ASTMeta::get()->builder_.get();
    }

    static Module *getModule()
    {
        return ASTMeta::get()->module_.get();
    }

    static NamedValues *getValues()
    {
        return ASTMeta::get()->values_.get();
    }

private:
    ASTMeta() = default;

    std::unique_ptr<Context> context_;
    std::unique_ptr<Builder> builder_;
    std::unique_ptr<Module> module_;
    std::unique_ptr<NamedValues> values_;
};


// AST types

class ExpressionAST {
public:

    virtual llvm::Value *codeGen() const = 0;

    virtual ~ExpressionAST() = default;
};

// Loggers

class PrototypeAST;

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
                *ASTMeta::getCtx(),
                llvm::APInt(64, value_));
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
        llvm::Value *value = (*ASTMeta::getValues())[name_];
        if (!value) {
            std::string msg = "Unknown variable name " + name_;
            LogErrorV(msg.c_str());
        }
        return value;
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
            return ASTMeta::getBuilder()->CreateFAdd(lhs, rhs, "addtmp");
        }
        if (op_ == "-") {
            return ASTMeta::getBuilder()->CreateFSub(lhs, rhs, "rhssubtmp");
        }
        if (op_ == "*") {
            return ASTMeta::getBuilder()->CreateFMul(lhs, rhs, "rhsmultmp");
        }
        if (op_ == "/") {
            return ASTMeta::getBuilder()->CreateFDiv(lhs, rhs, "rhsdivtmp");
        }
        if (op_ == "<") {
            return less(lhs, rhs);
        }
        if (op_ == ">") {
            return less(rhs, lhs);
        }
        std::string msg = "invalid binary operator ";
        msg += op_;
        msg += ("for " + lhs->getName() + " | " + rhs->getName()).str();
        return LogErrorV(msg.c_str());
    }

private:
    llvm::Value *less(llvm::Value *lhs, llvm::Value *rhs) const
    {
        lhs = ASTMeta::getBuilder()->CreateFCmpULT(lhs, rhs, "cmptmp");
        return ASTMeta::getBuilder()->CreateUIToFP(
                lhs,
                llvm::Type::getDoubleTy(*ASTMeta::getCtx()),
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
        llvm::Function *calleeF = ASTMeta::getModule()->getFunction(callee_);
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

        return ASTMeta::getBuilder()->CreateCall(
                calleeF,
                argsValues,
                "calltmp");
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

    llvm::Value *codeGen() const
    {
        std::vector<llvm::Type *> argsTypes(args_.size(), getType());

        llvm::Function *func =
                llvm::Function::Create(
                        llvm::FunctionType::get(getType(), argsTypes, false),
                        llvm::Function::ExternalLinkage,
                        name_,
                        ASTMeta::getModule());

        for (auto arg : std::views::zip(func->args(), args_)) {
            std::get<0>(arg).setName(std::get<1>(arg));
        }

        return func;
    }

private:
    static llvm::IntegerType *getType()
    {
        return llvm::Type::getInt64Ty(*ASTMeta::getCtx());
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

    llvm::Value *codeGen() const
    {
        llvm::Function *func = ASTMeta::getModule()->getFunction(proto_->getName());

        if (!func) {
            return nullptr;
        }

        if (!func->empty()) {
            std::string msg = "Redefinition of function " + proto_->getName();
            return (llvm::Function *)LogErrorV(msg.c_str());
        }

        ASTMeta::getBuilder()->SetInsertPoint(
                llvm::BasicBlock::Create(
                        *ASTMeta::getCtx(),
                        "entry",
                        func));

        ASTMeta::getValues()->clear();
        for (auto &arg : func->args()) {
            (*ASTMeta::getValues())[std::string(arg.getName())] = &arg;
        }

        if (llvm::Value *retVal = body_->codeGen()) {
            ASTMeta::getBuilder()->CreateRet(retVal);
            llvm::verifyFunction(*func);
            return func;
        }

        func->eraseFromParent();
        return nullptr;
    }

private:
    std::unique_ptr<PrototypeAST> proto_;
    std::unique_ptr<ExpressionAST> body_;
};

} // namespace AST

namespace Token {

int getTokenPrecedence(const std::string& token) // NOLINT
{
    static std::map<std::string, int> binopPrecedence = {
        { "=",  0 },

        { "<", 10 },
        { ">", 10 },

        { "+", 20 },
        { "-", 20 },

        { "*", 40 },
        { "/", 40 },
    };

    if (binopPrecedence.contains(token)) {
        return binopPrecedence.at(token);
    }
    return -1;
}

} // namespace Token

namespace Parser {

class Parser {
public:
    Parser()
        : token_(Token::_NULL, nullptr)
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

        // 'call'
        getToken();
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
        if (token_.first == Token::IDENT) {
            if (getTokenName() == "(") {
                return parseParentheses();
            }
            return parseIdentifier();
        }
        if (token_.first == Token::INT) {
            return parseValue();
        }
        return AST::LogError("Unknown token when expecting an expression");
    }

    std::unique_ptr<AST::ExpressionAST> parseExpression()
    {
        auto lhs = parsePrimary();
        if (!lhs) {
            return nullptr;
        }
        return parseRhsBinOp(0, std::move(lhs));
    }

    // expression
    // seq(@expression, @bin_op, @expression)
    std::unique_ptr<AST::ExpressionAST> parseRhsBinOp(
            int precedence,
            std::unique_ptr<AST::ExpressionAST> lhs)
    {
        // parse rhs while operators precedence lower than current op
        // while (Token::getTokenPrecedence(getTokenName()) < precedence) {
        while (true) {
            int TokPrec = Token::getTokenPrecedence(getTokenName());

            // If this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done.
            if (TokPrec < precedence)
                return lhs;

            std::string op = getTokenName();
            getToken();

            // if (op == ";") break;

            auto rhs = parsePrimary();
            if (!rhs) {
                return nullptr;
            }

            // if op binds less tightly with rhs than the operator after rhs
            // let the pending operator take rhs as its lhs
            int nextPrecedence = Token::getTokenPrecedence(getTokenName());
            if (precedence < nextPrecedence) {
                rhs = parseRhsBinOp(precedence + 1, std::move(rhs));
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
        if (token_.first != Token::IDENT) {
            return AST::LogErrorP("Expected function name in prototype");
        }

        std::string name = getTokenName();
        getToken();

        if (getTokenName() != "(") {
            return AST::LogErrorP("Expected '(' in prototype");
        }

        std::vector<std::string> args;
        while (getToken().first == Token::IDENT && getTokenName() != ")") {
            args.push_back(getTokenName());

            if (getTokenName() != ",") {
                return AST::LogErrorP("Expected ',' between args in args list prototype");
            }
            getToken(); // eject ','
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
    std::unique_ptr<AST::FunctionAST> parseDefenition()
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
        if (!expr) {
            return nullptr;
        }
        auto proto = std::make_unique<AST::PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<AST::FunctionAST>(std::move(proto), std::move(expr));
    }

    // Handlers
    void HandleDefinition()
    {
        if (parseDefenition()) {
            fprintf(stderr, "-- Parsed a function definition.\n");
        }
        else {
            getToken();
        }
    }

    void HandleExtern()
    {
        if (parseExtern()) {
            fprintf(stderr, "-- Parsed an extern\n");
        }
        else {
            getToken();
        }
    }

    void HandleTopLevelExpression()
    {
        if (parseTopLevelExpr()) {
            fprintf(stderr, "-- Parsed a top-level expr\n");
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
        fprintf(stderr, "ready> ");
        getToken();

        while (true) {
            if (token_.first == Token::END) {
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
            fprintf(stderr, "ready> ");
        }
    }
private:
    Token::TokenData getToken()
    {
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
        return getTokenData<std::string>();
    }

    Token::Tokenizer tokenizer_;
    Token::TokenData token_;
};

} // namespace Parser
