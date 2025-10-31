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

std::ostream& operator<<(std::ostream& cout, const std::vector<size_t>& trace)
{
    std::string delim = "";
    for (size_t stateIndex : trace)
    {
        cout << delim << stateIndex;
        delim = ", ";
    }
    return cout;
}

int main()
{
    try
    {
        // "my name is "['a'-'z']['A'-'Z']|*·
        Regex::Flat::Type expr1 = {  
            Regex::Flat::Charset_t{ 'a', 'z', false },
            Regex::Flat::Charset_t{ 'A', 'Z', false },
            Regex::Flat::Union_t{ },
            Regex::Flat::Charset_t{ 'a', 'z', false },
            Regex::Flat::Charset_t{ 'A', 'Z', false },
            Regex::Flat::Union_t{ },
            Regex::Flat::KleeneStar_t{ },
            Regex::Flat::Concat_t{ }
        };

        NFA nfa = NFABuilder::Build<Regex::ItOrder::POST>({expr1});
        DrawStateMachine(nfa, "output/nfa.dot");

        DFA m(nfa);
        DrawStateMachine(m, "output/dfa.dot");
    
        for (const DFA::State& state : m.States())
        {
            std::cout << state.index << " : " 
                << state.caseTag << std::endl;
        }


        auto traceNTag = [&](size_t testNo, std::string_view str)
        {
            auto trace = m.Trace(str);
            size_t caseTag = m.Match(str);
            std::cout << testNo << ". " << trace << " -> "
                << caseTag << std::endl; 
        };

        traceNTag(1, "ab*");
        traceNTag(2, "");
        traceNTag(3, "defg");
        traceNTag(4, "ab");
        traceNTag(5, "123Test123");

    }
    catch ( std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
}
