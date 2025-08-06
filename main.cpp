#include "parser.h"

#include <iostream>

template <class... Args>
void write(Args&&... args)
{
    ((std::cout << args << ' '), ...) << std::endl;
}

int main()
{
    // Token::Tokenizer tokenizer;
    // while (true) {
    //     auto [token, data] = tokenizer.getToken();
    //     std::cout << Token::tokenToString(token);
    //     if (data) {
    //         if (token == Token::INT) {
    //             std::cout << *(int*)data;
    //         }
    //         else {
    //             std::cout << *(std::string*)data;
    //         }
    //     }
    //     std::cout << std::endl;
    //     std::cout << " --------- " << std::endl;
    //     if (token == Token::END) {
    //         break;
    //     }
    // }

    auto parser = Parser::Parser();
    parser.MainLoop();
}
