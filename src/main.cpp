/// @file main.cpp

#include "RuleCase.hpp"
#include "PreProcessor.hpp"
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






    // try
    // {
    //     RuleCase pattern1 {
    //         .patternData = "[a-z]*",
    //         .patternType = RuleCase::Pattern_t::REGEX,
    //         .matchAlias = "",
    //         .actionCode = ""
    //     };

    //     RuleCase pattern2 {
    //         .patternData = "[A-Z]*",
    //         .patternType = RuleCase::Pattern_t::REGEX,
    //         .matchAlias = "",
    //         .actionCode = ""
    //     };

    //     RuleCase pattern3 {
    //         .patternData = "[0-9]*",
    //         .patternType = RuleCase::Pattern_t::REGEX,
    //         .matchAlias = "",
    //         .actionCode = ""
    //     };

    //     RuleCase pattern4 {
    //         .patternData = "(dog)|(cat)",
    //         .patternType = RuleCase::Pattern_t::REGEX,
    //         .matchAlias = "",
    //         .actionCode = ""
    //     };

    //     RuleCase pattern5 {
    //         .patternData = "doggo",
    //         .patternType = RuleCase::Pattern_t::REGEX,
    //         .matchAlias = "",
    //         .actionCode = ""
    //     };

    //     NFA n = NFABuilder::Build({pattern2, pattern3, pattern4, pattern5});
    //     DrawStateMachine(n, "output/nfa.dot");
    //     DFA m(n);
    //     DrawStateMachine(m, "output/dfa.dot");
    //     // DFA::Minimize(m);
    //     // DrawStateMachine(m, "output/dfaMin.dot");
    // }
    // catch(std::exception& e)
    // {
    //     DBG << e.what() << std::endl;
    // }
}
