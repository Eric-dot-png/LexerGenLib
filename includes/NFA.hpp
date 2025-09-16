/// @file NFA.hpp
/// @brief Provides defnitions for NFA

#pragma once

#include <vector>
#include <cstddef>
#include <unordered_set>

/// @brief Represents a Non-deterministic Finite Automaton (NFA)
struct NFA
{
    /// @brief  Represents a transition from one state to another
    struct Transition
    {
        char symbol; ///< The input symbol for the transition
        size_t to; ///< The index of the result state
    };

    /// @brief Represents a state in the NFA
    struct State
    {
        const size_t index; ///< The index of the state
        const size_t ruleTag; ///< The rule tag associated with the state (if any)
        std::vector<Transition> transitions; ///< the transitions from this state
    };

    const size_t start; ///< The index of the start state
    const std::unordered_set<size_t> accept; ///< The indices of the accept states
    const std::vector<State> states; ///< The states of the NFA
};
