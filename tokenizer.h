#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>


namespace Token {

enum Token {
    _NULL   = 0,

    END     = -1, // EOF

    // -- Commands
    FUNC    = -2,   // def of func
    EXT     = -3,   // extern of module
    RET     = -4,   // func return

    // -- Values
    // const literals
    // BOOL    = -5,
    INT     = -7,
    // FLOAT   = -8,
    // STR     = -9,
    IDENT   = -10,   // identifier

    // -- Definitions
    // Types
    // TBOOL   = -11,
    // TINT    = -12,
    // TFLOAT  = -13,
    // TSTR    = -14,
};

typedef std::pair<Token, void*> TokenData;

std::string tokenToString(Token token);

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
