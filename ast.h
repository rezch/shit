#pragma once

#include "context.h"
#include "debug.h"

#include "llvm/IR/Value.h"

#include <cstdint>
#include <string>


namespace AST {

llvm::Function *findFunction(std::string name);

class ExpressionAST {
public:
    virtual llvm::Value *codeGen() const         = 0;
    virtual void debugPrint(int layer = 0) const = 0;

    virtual ~ExpressionAST() = default;
};

// Values

class ValueAST : public ExpressionAST {
public:
    ValueAST(int64_t value);

    llvm::Value *codeGen() const override;

    void debugPrint(int layer = 0) const override;

private:
    int64_t value_;
};

class VarAST : public ExpressionAST {
public:
    VarAST(std::string name);

    llvm::Value *codeGen() const override;

    void debugPrint(int layer = 0) const override;

private:
    std::string name_;
};

// Operators

class BinaryExprAST : public ExpressionAST {
public:
    BinaryExprAST(
            std::string op,
            std::unique_ptr<ExpressionAST> lhs,
            std::unique_ptr<ExpressionAST> rhs);

    llvm::Value *codeGen() const override;

    void debugPrint([[ maybe_unused ]] int layer = 0) const override;

    std::string getOp() const;

private:
    llvm::Value *less(llvm::Value *lhs, llvm::Value *rhs) const;

    std::string op_;
    std::unique_ptr<ExpressionAST> lhs_, rhs_;
};

// Functions

class CallExprAST : public ExpressionAST {
public:
    CallExprAST(
            std::string callee,
            std::vector<std::unique_ptr<ExpressionAST>> args);

    llvm::Value *codeGen() const override;

    void debugPrint(int layer = 0) const override;

private:
    std::string callee_;
    std::vector<std::unique_ptr<ExpressionAST>> args_;
};

class PrototypeAST {
public:
    PrototypeAST(
            std::string name,
            std::vector<std::string> args);

    const std::string &getName() const;

    llvm::Function *codeGen() const;

    void debugPrint(int layer = 0) const;

private:
    static llvm::IntegerType *getType()
    {
        return llvm::Type::getInt64Ty(*Context::IRManager::getCtx());
    }

    std::string name_;
    std::vector<std::string> args_;
};

class FunctionAST {
public:
    FunctionAST(
            std::unique_ptr<PrototypeAST> proto,
            std::unique_ptr<ExpressionAST> body);

    llvm::Function *codeGen();

    void debugPrint(int layer = 0) const;

private:
    std::unique_ptr<PrototypeAST> proto_;
    std::unique_ptr<ExpressionAST> body_;
};

// Statements
class IfElseExpressionAST : public ExpressionAST {
public:
    IfElseExpressionAST(
        std::unique_ptr<ExpressionAST> condExpr,
        std::unique_ptr<ExpressionAST> thenExpr,
        std::unique_ptr<ExpressionAST> elseExpr);

    llvm::Value *codeGen() const override;

    void debugPrint(int layer = 0) const override;

private:
    std::unique_ptr<ExpressionAST> condExpr_;
    std::unique_ptr<ExpressionAST> thenExpr_;
    std::unique_ptr<ExpressionAST> elseExpr_;
};

// Loggers

std::unique_ptr<ExpressionAST> LogError(const char *Str);
llvm::Value *LogErrorV(const char *str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *str);

} // namespace AST
