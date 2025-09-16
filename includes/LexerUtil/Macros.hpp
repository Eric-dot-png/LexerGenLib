/// @file Macros.hpp
/// @brief Common macros used across the project
/// @note This file should NOT be included in header files to avoid macro pollution

#pragma once

#include <stdexcept>
#include <stdexcept>
#include <format>

// Stringification helpers
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Expect/Ensure macros
#define EXPECTS_THROW(xCond, xMsg) \
    if (!(xCond)) { \
        throw std::invalid_argument(std::format("Error: {}:{} - {}", __FILE__, TOSTRING(__LINE__), xMsg)); \
    }

#define ENSURES_THROW(xCond, xMsg) \
    if (!(xCond)) { \
        throw std::invalid_argument(std::format("Error: {}:{} - {}", __FILE__, TOSTRING(__LINE__), xMsg)); \
    }
