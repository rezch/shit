#include "parser.h"

#include <iostream>

template <class... Args>
void write(Args&&... args)
{
    ((std::cerr << args << ' '), ...) << std::endl;
}

int main()
{
    // Token::Tokenizer tokenizer;
    // while (true) {
    //     auto [token, data] = tokenizer.getToken();
    //     std::cerr << Token::tokenToString(token);
    //     if (data) {
    //         if (token == Token::INT) {
    //             std::cerr << *(int*)data;
    //         }
    //         else {
    //             std::cerr << *(std::string*)data;
    //         }
    //     }
    //     std::cerr << std::endl;
    //     std::cerr << " --------- " << std::endl;
    //     if (token == Token::END) {
    //         break;
    //     }
    // }

    auto parser = Parser::Parser();
    parser.MainLoop();
}
