/** @file Regex.cpp
 *  @brief Implementations for regex related structs, methods, and functions
 */

#include "Regex.hpp"


using namespace Regex;


inline 
std::ostream& toStream(std::ostream& os, const Flat::Char_t& re)
{
   return (os << '\'' << re.value << '\'');
}


inline 
std::ostream& toStream(std::ostream& os, const Flat::Literal_t& re)
{
    return (os << '"' << re.value << '"');
}


std::ostream& toStream(std::ostream& os, const Flat::Charset_t& re)
{
    os << '[' << (re.inverted ? "^" : "");
    std::string delim = "";
    for (char c : re.chars)
    {
        os << delim << c;
        delim = ", ";
    }
    os << ']';
    return os;
}


inline 
std::ostream& toStream(std::ostream& os, const Flat::Concat_t&)
{
    return (os << "·");
}


inline
std::ostream& toStream(std::ostream& os, const Flat::Union_t&)
{
    return (os << '|');
}


inline
std::ostream& toStream(std::ostream& os, const Flat::KleeneStar_t&)
{
    return (os << '*');
}


std::ostream &operator<<(std::ostream & os, const Flat::Symbol & symbol)
{
    return std::visit([&os](auto&& arg) -> std::ostream& {
       return toStream(os, arg);
    }, symbol);
}


std::ostream &operator<<(std::ostream & os, const Flat::Type & symbols)
{
    for (const auto& symbol : symbols)
    {
        os << symbol;
    }

    return os;
}
