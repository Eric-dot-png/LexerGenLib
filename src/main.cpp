/// @file main.cpp

#include "RuleCase.hpp"
#include "PreProcessor.hpp"
#include "NFABuilder.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"
#include "DFA.hpp"

#include <iostream>
#include <fstream>

template <typename T>
static void DrawStateMachine(const T& sm, const char * const outFilePath)
{
    std::fstream file(outFilePath, std::ios::out);
    ENSURES_THROW(file.is_open(), std::format("Could not open file '{}'", outFilePath));


    std::unordered_map<size_t, std::unordered_map<size_t, std::string>> labelMap;
    
    file << "digraph StateMachine {\n"
        << "    rankdir=LR;\n"
        << "    node [shape=circle];\n\n\n";

    for (const auto& state : sm.States())
    {
        std::string shape = (sm.Accepting().contains(state.index) ? "doublecircle" : "circle");
        file << std::format("q{} [shape={}, label=\"{}\"];\n", state.index, shape, state.index);
    }

    for (const auto& state : sm.States())
    {
        labelMap[state.index];
        for (const auto& [symbol, result] : state.transitions)
        {
            labelMap[state.index][result] += Escaped(symbol);
        }
        for (const auto& [result, label] : labelMap[state.index])
        {
            file << std::format("q{} -> q{} [label=\"{}\"];\n", state.index, result, label);
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
        DrawStateMachine(n, "output/nfa.dot");
        DFA m(n);
        DrawStateMachine(m, "output/dfa.dot");
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
