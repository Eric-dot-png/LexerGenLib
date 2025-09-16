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

auto NFABuilder::ApplyCAT(const Fragment &left, const Fragment &right,
    std::vector<NFA::State>& states) -> NFABuilder::Fragment
{
    PatchHoles(right.startIndex, left.holes, states);    
    return Fragment{left.startIndex, right.holes };
}

void NFABuilder::PatchHoles(size_t patchState, 
    const std::vector<Fragment::Hole> &holes, std::vector<NFA::State> &states)
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
