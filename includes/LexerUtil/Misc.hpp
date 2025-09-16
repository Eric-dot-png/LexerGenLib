/// @file Misc.hpp
/// @brief Miscellaneous utility functions for the lexer

#pragma once

#include <stack>
#include <string>
#include <sstream>

template <typename T>
T pop(std::stack<T>& stk) 
{
    T val = stk.top();
    stk.pop();
    return val;
}
