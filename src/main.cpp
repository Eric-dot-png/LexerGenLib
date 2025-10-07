/// @file main.cpp

#include "RuleCase.hpp"
#include "PreProcessor.hpp"
#include "NFABuilder.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Constants.hpp"

#include <iostream>
#include <fstream>

template <typename T>
static void DrawStateMachine(const std::vector<T>& states, const char * const outFilePath)
{
    std::fstream file(outFilePath, std::ios::out);
    ENSURES_THROW(file.is_open(), std::format("Could not open file '{}'", outFilePath));

    file << "digraph StateMachine {\n"
        << "    rankdir=LR;\n"
        << "    node [shape=circle];\n\n\n";


    for (const T& state : states)
    {
        for (const auto& transition : state.transitions)
        {
            file << "    q" << state.index << "->"
                << "q" << transition.to;
            
            if (transition.symbol != EPSILON)
            {
                file << std::format("[label=\"{}\"];", transition.symbol);
            }
            else
            {
                file << "[label=\"ε\"];"; 
            }
            file << '\n';
        }
    }

    file << "}\n";
}

int main()
{
    try
    {
        RuleCase pattern {
            .patternData = "0123456789_",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        PreProcessor::PreProcess(pattern);
        NFA n = NFABuilder::Build({pattern});
        std::cout << n.states.size() << std::endl;
        DrawStateMachine(n.states, "output/fsm.dot");
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
