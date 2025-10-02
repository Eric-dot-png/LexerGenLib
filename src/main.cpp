/// @file main.cpp

#include "RuleCase.hpp"
#include "PreProcessor.hpp"
#include "NFABuilder.hpp"

#include <iostream>

int main()
{
    try
    {
        RuleCase pattern {
            .patternData = "[0-9]*[a-zA-Z\\^]+",
            .patternType = RuleCase::Pattern_t::REGEX,
            .matchAlias = "",
            .actionCode = ""
        };

        PreProcessor::PreProcess(pattern);
        NFA n = NFABuilder::Build({pattern});
        
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
