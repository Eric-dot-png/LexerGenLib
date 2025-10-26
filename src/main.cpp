/// @file main.cpp

#include "NFABuilder.hpp"
#include "DFA.hpp"
#include "NFA.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"
#include "LexerUtil/Drawing.hpp"

#include "Regex.hpp"

#include <iostream>

int main()
{
    try
    {
        Regex::Flat::Type expr = {  
            Regex::Flat::Char_t{ 'a' },
            Regex::Flat::Char_t{ 'b' },
            Regex::Flat::Concat_t{ },
            Regex::Flat::Charset_t{ 'd', 'z', false },
            Regex::Flat::KleeneStar_t{ },
            Regex::Flat::Union_t{ },
            Regex::Flat::Literal_t{ "Bannana * Apple"},
            Regex::Flat::Concat_t{ }
        };

        auto nfa = NFABuilder::Build<Regex::ItOrder::POST>({expr});
        DrawStateMachine(nfa, "output/nfa.dot");

        DFA m(nfa);
        DrawStateMachine(m, "output/dfa.dot");
    }
    catch ( std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
}
