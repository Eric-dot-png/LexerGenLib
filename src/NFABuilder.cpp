/// @file NFABuilder.cpp
/// @brief NFABuilder definitions

#include "NFABuilder.hpp"
#include "NFA.hpp"
#include "LexerUtil/Constants.hpp"

NFA NFABuilder::Build(const std::vector<Pattern> & preProcessedPatterns)
{
    (void)preProcessedPatterns;

    NFA ret{ 
        .start = INVALID_STATE_INDEX,
        .accept = {},
        .states = {}
    };

    return ret;
}

size_t NFABuilder::NewState(std::vector<NFA::State> &nfaStates)
{
    return NFABuilder::NewState(NO_RULE_TAG, nfaStates);   
}

size_t NFABuilder::NewState(size_t ruleNo, std::vector<NFA::State> &nfaStates)
{
    size_t stateIndex = nfaStates.size();
    nfaStates.push_back(NFA::State{
        .index = stateIndex,
        .ruleTag = ruleNo,
        .transitions = {}
    });
    return stateIndex;
}

auto NFABuilder::MakeLiteral(char a, std::vector<NFA::State> &nfaStates)
    -> NFABuilder::Fragment
{
    size_t q0 = NewState(nfaStates);
    
    return Fragment{
        .startIndex = q0, 
        .holes = { 
            Fragment::Hole{
                .holeIndex = q0,
                .tVal = a
            } 
        } 
    };
}

auto NFABuilder::ApplyCat(const Fragment &left, const Fragment &right,
    std::vector<NFA::State>& states) -> NFABuilder::Fragment
{
    PatchHoles(left.holes, right.startIndex, states);    
    return Fragment{left.startIndex, right.holes };
}

auto NFABuilder::ApplyUnion(const Fragment &left, const Fragment &right,
    std::vector<NFA::State>& states) -> NFABuilder::Fragment
{
    /// add a new state and perform the union on the fragments
    ///
    size_t newStateIndex = NewState(states);
    states[newStateIndex].transitions = {
        NFA::Transition{
            .symbol = EPSILON,
            .to = left.startIndex
        },
        NFA::Transition{
            .symbol = EPSILON,
            .to = right.startIndex
        }
    };

    Fragment ret{
        .startIndex = newStateIndex,
        .holes = {}
    };

    /// insert the holes of both fragments into the new fragment & return
    ///
    ret.holes.reserve(left.holes.size() + right.holes.size());
    ret.holes.insert(ret.holes.end(), left.holes.begin(), left.holes.end());
    ret.holes.insert(ret.holes.end(), left.holes.begin(), left.holes.end());
    
    return ret;
}

void NFABuilder::PatchHoles(const std::vector<Fragment::Hole> &holes, 
    size_t patchState, std::vector<NFA::State> &states)
{
    for (const auto& hole : holes)
    {
        states[hole.holeIndex].transitions.push_back(
            NFA::Transition{
                .symbol = hole.tVal, 
                .to = patchState
            }
        );
    }
}
