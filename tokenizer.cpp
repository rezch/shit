#include "tokenizer.h"

#include <algorithm>


namespace Token {

namespace {

bool isInt(const std::string& str)
{
    return std::find_if(str.begin(), str.end(),
                [](char c) { return !std::isdigit(c); }
            ) == str.end();
}

} // namespace

int getTokenPrecedence(const std::string& token)
{
    if (binopPrecedence.contains(token)) {
        return binopPrecedence.at(token);
    }
    return -1;
}

void freeToken(TokenData token)
{
    if (token.first == TokenType::IDENT) {
        delete static_cast<std::string*>(token.second);
    }
    if (token.first == TokenType::INT) {
        delete static_cast<int64_t*>(token.second);
    }
}

std::string tokenToString(TokenType token)
{
    switch (token) {
        case END    :   return "TOKEN : END";
        case FUNC   :   return "TOKEN : FUNC";
        case EXT    :   return "TOKEN : EXT";
        case RET    :   return "TOKEN : RET";
        case INT    :   return "TOKEN : INT";
        case IDENT  :   return "TOKEN : IDENT";
        case IF     :   return "TOKEN : IF";
        case ELSE   :   return "TOKEN : ELSE";
        case FOR    :   return "TOKEN : FOR";
    }
}

Tokenizer::Tokenizer()
    : lastSym(' ')
{ }

TokenData Tokenizer::getToken()
{
    auto [token, data] = getTokenImpl();
    return { token, data };
}

TokenData Tokenizer::getTokenImpl()
{
    auto currentIdent = parseToken();

    if (currentIdent == "") {
        return { TokenType::END, nullptr };
    }
    if (currentIdent == "fun") {
        return { TokenType::FUNC, nullptr };
    }
    if (currentIdent == "extern") {
        return { TokenType::EXT, nullptr };
    }
    if (currentIdent == "ret") {
        return { TokenType::RET, nullptr };
    }
    if (currentIdent == "if") {
        return { TokenType::IF, nullptr };
    }
    if (currentIdent == "else") {
        return { TokenType::ELSE, nullptr };
    }
    if (currentIdent == "for") {
        return { TokenType::FOR, nullptr };
    }
    return parseValue(currentIdent);
}

void Tokenizer::skipComment() const
{
    char sym = '\0';
    while (sym != EOF && sym != '\n' && sym != '\r') {
        sym = getchar();
    }
}

std::string Tokenizer::parseToken()
{
    // skip spaces
    while (std::isspace(lastSym) || lastSym == '\n') {
        lastSym = std::getchar();
    }

    if (lastSym == '#') {
        skipComment();
        lastSym = getchar();
    }

    if (lastSym == EOF) { return ""; }

    if (std::isalnum(lastSym)) {
        return readWord();
    }

    std::string sym{lastSym};
    lastSym = getchar();
    return sym;
}

std::string Tokenizer::readWord()
{
    std::string currentIdent = {};
    do {
        currentIdent += lastSym;
        lastSym = getchar();
    } while (isalnum(lastSym) && lastSym != EOF && lastSym != ' ' && lastSym != ';' && lastSym != '\n');
    return currentIdent;
}

TokenData Tokenizer::parseValue(const std::string& value)
{
    if (isInt(value)) {
        return { TokenType::INT, new int64_t(std::stoll(value)) };
    }
    // if (isFloat(value)) {
    //     return { TokenType::FLOAT, new double(std::stod(value)) };
    // }
    // if (value[0] == '"') {
    //     return { TokenType::STR, new std::string(value.substr(1, value.size() - 2)) };
    // }
    // if (value == "true" || value == "false") {
    //     return { TokenType::BOOL, new bool(value == "true") };
    // }
    return { TokenType::IDENT, new std::string(value) };
}

} // namespace Token
