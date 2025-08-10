#include "parser.h"

extern "C" {
#include "libstd.h"
}


int main()
{
    auto parser = Parser::Parser();
    parser.MainLoop();
}
