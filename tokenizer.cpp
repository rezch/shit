#include "tokenizer.h"

#include <algorithm>
#include <iostream>


namespace Token {

namespace {

bool isInt(const std::string& str)
{
    return std::find_if(str.begin(), str.end(),
                [](char c) { return !std::isdigit(c); }
            ) == str.end();
}

// bool isFloat(const std::string& str)
// {
//     bool countDot = false;
//     return std::find_if(str.begin(), str.end(),
//             [&countDot](char c) {
//                 if (c != '.')
//                     return !std::isdigit(c);
//                 if (countDot)
//                     return true;
//                 countDot = true;
//                 return false;
//             }) == str.end();
// }

} // namespace

void freeToken(TokenData token)
{
    if (token.first == Token::IDENT) {
        delete static_cast<std::string*>(token.second);
    }
    if (token.first == Token::INT) {
        delete static_cast<int64_t*>(token.second);
    }
}

std::string tokenToString(Token token)
{
    switch (token) {
        case END    :   return "TOKEN : END";
        case FUNC   :   return "TOKEN : FUNC";
        case EXT    :   return "TOKEN : EXT";
        case RET    :   return "TOKEN : RET";
        // case BOOL   :   return "TOKEN : BOOL";
        case INT    :   return "TOKEN : INT";
        // case FLOAT  :   return "TOKEN : FLOAT";
        // case STR    :   return "TOKEN : STR";
        case IDENT  :   return "TOKEN : IDENT";
        // case TBOOL  :   return "TOKEN : TBOOL";
        // case TINT   :   return "TOKEN : TINT";
        // case TFLOAT :   return "TOKEN : TFLOAT";
        // case TSTR   :   return "TOKEN : TSTR";
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
        return { Token::END, nullptr };
    }
    if (currentIdent == "fun") {
        return { Token::FUNC, nullptr };
    }
    if (currentIdent == "extern") {
        return { Token::EXT, nullptr };
    }
    if (currentIdent == "ret") {
        return { Token::RET, nullptr };
    }
    // if (currentIdent == "bool") {
    //     return { Token::TBOOL, nullptr };
    // }
    // if (currentIdent == "int") {
    //     return { Token::TINT, nullptr };
    // }
    // if (currentIdent == "float") {
    //     return { Token::TFLOAT, nullptr };
    // }
    // if (currentIdent == "str") {
    //     return { Token::TSTR, nullptr };
    // }
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
    while (isalnum(lastSym) && lastSym != EOF && lastSym != ' ' && lastSym != ';' && lastSym != '\n') {
        currentIdent += lastSym;
        lastSym = getchar();
    }
    return currentIdent;
}

TokenData Tokenizer::parseValue(const std::string& value)
{
    if (isInt(value)) {
        return { Token::INT, new int64_t(std::stoll(value)) };
    }
    // if (isFloat(value)) {
    //     return { Token::FLOAT, new double(std::stod(value)) };
    // }
    // if (value[0] == '"') {
    //     return { Token::STR, new std::string(value.substr(1, value.size() - 2)) };
    // }
    // if (value == "true" || value == "false") {
    //     return { Token::BOOL, new bool(value == "true") };
    // }
    return { Token::IDENT, new std::string(value) };
}

} // namespace Token
