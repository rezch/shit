#pragma once

#include "debug.h"

#include <map>
#include <string>


namespace Token {

enum TokenType {
    END     = -1, // EOF

    // -- Commands
    FUNC    = -2,   // def of func
    EXT     = -3,   // extern of module
    RET     = -4,   // func return

    // -- Values
    INT     = -7,
    IDENT   = -10,  // identifier

    // -- Statements
    // if/else
    IF      = -11,
    ELSE    = -12,
};

static const std::map<std::string, int> binopPrecedence = {
    { "=",  0 },

    { "<", 10 },
    { ">", 10 },

    { "+", 20 },
    { "-", 20 },

    { "*", 40 },
    { "/", 40 },
};

int getTokenPrecedence(const std::string& token);

typedef std::pair<TokenType, void*> TokenData;

void freeToken(TokenData token);

std::string tokenToString(TokenType token);

class Tokenizer {
public:

    Tokenizer();

    TokenData getToken();

private:
    TokenData getTokenImpl();

    std::string parseToken();

    std::string readWord();

    void skipComment() const;

    TokenData parseValue(const std::string& value);

    char lastSym;
};

} // namespace Token
