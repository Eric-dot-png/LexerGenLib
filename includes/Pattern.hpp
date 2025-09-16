/// @file Pattern.hpp

#pragma once

#include <tuple>
#include <string_view>

/// @brief struct to represent a pattern
struct Pattern
{
    std::string pattern; ///< The pattern string
    bool isRegex; ///< true if the pattern is a regex, false if it's a literal string
};

inline Pattern Regex(std::string pattern) { return Pattern{pattern, true};}
inline Pattern String(std::string pattern) { return Pattern{pattern, false};}
