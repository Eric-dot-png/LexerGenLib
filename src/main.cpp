/// @file main.cpp

#include <iostream>
#include "Pattern.hpp"
#include "PreProcessor.hpp"

int main()
{
    try
    {
        Pattern pattern {
            .pattern = "[0-9]*[a-zA-Z^]+",
            .isRegex = true
        };

        PreProcessor::PreProcess(pattern);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
