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

inline std::string Escaped(char symbol)
{
    switch ( symbol )
    {
        case '\n' : return "\\n";
        case '[': return "\\[";
        case ']': return "\\]";
        case '"': return "\\\"";
        case '\'': return "\\'";
        case '\\': return "\\\\";
        case '\t': return "\\t";
        case EPSILON: return "ε";
        default: return std::string(1, symbol);
    }
}
