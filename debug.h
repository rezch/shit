#pragma once

#include <iostream>


#if DEBUG
template <class... Args>
void debug(int layer, Args &&...args)
{
    std::cerr << std::string(layer * 4, ' ');
    ((std::cerr << args << ' '), ...) << std::endl;
}
#define IFDEBUG(...) __VA_ARGS__
#else
template <class... Args>
void debug([[ maybe_unused ]] int layer, [[ maybe_unused ]] Args &&...args)
{ }
#define IFDEBUG(...) { }
#endif
