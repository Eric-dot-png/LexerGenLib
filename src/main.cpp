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
        std::string shape = (state.caseTag != NO_CASE_TAG ? "doublecircle" : "circle");
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
        RuleCase pattern1 {
            .patternData = "[a-z]*",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        RuleCase pattern2 {
            .patternData = "[A-Z]*",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        RuleCase pattern3 {
            .patternData = "[0-9]*",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        RuleCase pattern4 {
            .patternData = "(dog)|(cat)",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        RuleCase pattern5 {
            .patternData = "doggo",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        NFA n = NFABuilder::Build({pattern2, pattern3, pattern4, pattern5});
        DrawStateMachine(n, "output/nfa.dot");
        DFA m(n);
        DrawStateMachine(m, "output/dfa.dot");
        // DFA::Minimize(m);
        // DrawStateMachine(m, "output/dfaMin.dot");
    }
    catch(std::exception& e)
    {
        DBG << e.what() << std::endl;
    }
}
