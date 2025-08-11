#include "context.h"
#include "parser.h"
#include "jit.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"

#include <cstdint>
#include <map>
#include <string>


namespace Parser {

Parser::Parser()
    : token_(Token::END, nullptr)
{ }

std::unique_ptr<AST::ExpressionAST> Parser::parseValue()
{
    auto result = std::make_unique<AST::ValueAST>(getTokenData<int64_t>());
    getToken();
    return result;
}

std::unique_ptr<AST::ExpressionAST> Parser::parseParentheses()
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

std::unique_ptr<AST::ExpressionAST> Parser::parseIdentifier()
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

std::unique_ptr<AST::ExpressionAST> Parser::parsePrimary()
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

std::unique_ptr<AST::ExpressionAST> Parser::parseExpression()
{
    auto lhs = parsePrimary();
    if (!lhs) {
        return nullptr;
    }
    return parseRhsBinOp(0, std::move(lhs));
}

std::unique_ptr<AST::ExpressionAST> Parser::parseRhsBinOp(
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

std::unique_ptr<AST::PrototypeAST> Parser::parsePrototype()
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

std::unique_ptr<AST::FunctionAST> Parser::parseDefinition()
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

std::unique_ptr<AST::PrototypeAST> Parser::parseExtern()
{
    getToken(); // eject 'extern'
    return parsePrototype();
}

std::unique_ptr<AST::FunctionAST> Parser::parseTopLevelExpr()
{
    auto expr = parseExpression();
    // if (isAssigment(expr)) {
    //     debug(0, "ASSIGMENT");
    //     return expr;
    // }
    if (!expr) {
        return nullptr;
    }
    debug(0, "ADD ANON EXPR");
    auto proto = std::make_unique<AST::PrototypeAST>("__anon_expr", std::vector<std::string>());
    return std::make_unique<AST::FunctionAST>(std::move(proto), std::move(expr));
}

void Parser::HandleDefinition()
{
    auto funcAST = parseDefinition();
    funcAST->debugPrint();
    if (funcAST) {
        auto *funcIR = funcAST->codeGen();
        if (funcIR) {
            fprintf(stderr, "Read function definition:\n");
            funcIR->print(llvm::errs());
            fprintf(stderr, "\n");

            Context::IRManager::onErr(
                Context::IRManager::getJIT()->addModule(
                    llvm::orc::ThreadSafeModule(
                        Context::IRManager::moveModule(),
                        Context::IRManager::moveCtx())));

            Context::IRManager::reinit();
        }
    }
    else {
        getToken();
    }
}

void Parser::HandleExtern()
{
    auto protoAST = parseExtern();
    protoAST->debugPrint();
    if (protoAST) {
        auto *funcIR = protoAST->codeGen();
        if (funcIR) {
            fprintf(stderr, "Read extern:\n");
            funcIR->print(llvm::errs());
            fprintf(stderr, "\n");
            Context::IRManager::getFunctionProtos()[protoAST->getName()] = std::move(protoAST);
        }
    }
    else {
        getToken();
    }
}

void Parser::HandleTopLevelExpression()
{
    auto funcAST = parseTopLevelExpr();
    funcAST->debugPrint();
    if (funcAST) {
        if (funcAST->codeGen()) {
            auto retType = Context::IRManager::getJIT()->getMainJITDylib().createResourceTracker();

            auto tsm = llvm::orc::ThreadSafeModule(
                Context::IRManager::moveModule(),
                Context::IRManager::moveCtx());

            Context::IRManager::onErr(
                    Context::IRManager::getJIT()->addModule(std::move(tsm), retType));
            Context::IRManager::reinit();

            auto exprSymbol = Context::IRManager::onErr(
                Context::IRManager::getJIT()->lookup("__anon_expr"));

            int64_t (*intPtr)() = exprSymbol.toPtr<int64_t (*)()>();
            fprintf(stderr, "Evaluated to %ld\n", intPtr());

            Context::IRManager::onErr(retType->remove());
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
void Parser::MainLoop()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    Context::IRManager::reinit();

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

    Context::IRManager::getModule()->print(llvm::errs(), nullptr);
}

} // namespace Parser
