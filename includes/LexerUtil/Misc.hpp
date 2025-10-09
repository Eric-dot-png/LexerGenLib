/// @file Misc.hpp
/// @brief Miscellaneous utility functions for the lexer

#pragma once

#include <queue>
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

template <typename T>
T pop(std::queue<T>& queue)
{
    T val = queue.front();
    queue.pop();
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
