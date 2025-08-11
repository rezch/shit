#pragma once

#include "context.h"

#include "llvm/IR/Value.h"

#include <cstdint>
#include <iostream>
#include <string>


#if DEBUG
template <class... Args>
void debug(int layer, Args &&...args)
{
    std::cerr << std::string(layer * 4, ' ');
    ((std::cerr << args << ' '), ...) << std::endl;
}
#else
template <class... Args>
void debug(int layer, Args &&...args)
{ }
#endif

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

    void debugPrint(int layer = 0) const override;

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

// Loggers

std::unique_ptr<ExpressionAST> LogError(const char *Str);
llvm::Value *LogErrorV(const char *str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *str);

} // namespace AST
