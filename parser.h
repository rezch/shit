#pragma once

#include "ast.h"
#include "tokenizer.h"

#include <string>


namespace Parser {

class Parser {
public:
    Parser();

    // number
    std::unique_ptr<AST::ExpressionAST> parseValue();

    // parentheses
    // seq('(', optional(@expression), ')')
    std::unique_ptr<AST::ExpressionAST> parseParentheses();

    // identifier | call
    // @identifier
    // @call: seq(
    //          @identifier
    //          '(',
    //          repeat(
    //              seq(@identifier, optional(','))),
    //          ')')
    std::unique_ptr<AST::ExpressionAST> parseIdentifier();

    // primary
    // choice(@identifier,
    //        @call,
    //        @number,
    //        @parentheses)
    std::unique_ptr<AST::ExpressionAST> parsePrimary();

    // expression
    // seq(@expression, @bin_op, @expression)
    std::unique_ptr<AST::ExpressionAST> parseExpression();

    std::unique_ptr<AST::ExpressionAST> parseRhsBinOp(
        int exprPrec,
        std::unique_ptr<AST::ExpressionAST> lhs);

    // @prototype
    // seq(
    //     @identifier
    //     '(',
    //     repeat(
    //         seq(@identifier, optional(','))),
    //     ')')
    std::unique_ptr<AST::PrototypeAST> parsePrototype();

    // function_defenition
    // seq(
    //      defenition: @prototype,
    //      body:       @expression)
    std::unique_ptr<AST::FunctionAST> parseDefinition();

    // extern
    // seq(
    //      'extern',
    //      @prototype)
    std::unique_ptr<AST::PrototypeAST> parseExtern();

    // @expression
    std::unique_ptr<AST::FunctionAST> parseTopLevelExpr();

    // ---- Handlers
    void HandleDefinition();

    void HandleExtern();

    void HandleTopLevelExpression();

    // top
    // choice(
    //      @definition,
    //      @external,
    //      @expression)
    void MainLoop();

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
