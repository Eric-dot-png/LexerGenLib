/// @file Regex.hpp
/// @brief Provides Flattened Regex structure definitions and 
///        utility functions used for postorder traversal

#pragma once

#include <string_view>
#include <variant>
#include <vector>

namespace Regex
{
    /// @brief type of iteration over regex containers
    enum class ItOrder : size_t
    {
        PRE = 0,
        IN = 1,
        POST = 2
    };

    /// @brief NS for Flattened regex types. These types are not recursive (non-ast), and are meant for postorder traversal
    namespace Flat
    {
        /// 
        /// Terminal FlatRegex Types
        ///

        struct Char_t { char value; };
        struct Literal_t { std::string_view value; };
        struct Charset_t { char lo, hi; bool inverted; };

        ///
        /// Non-Terminal FlatRegex Types
        ///
        
        struct Union_t { };
        struct Concat_t { };
        struct KleeneStar_t{ };

        ///
        /// Flat Regex symbol type 
        ///
        using Symbol = std::variant<Char_t,Literal_t,Charset_t,Union_t,Concat_t,KleeneStar_t>;

        ///
        /// Flat regex expression type
        ///
        using Type = std::vector<Symbol>;
    };

};