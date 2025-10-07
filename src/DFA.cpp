/// @file DFA.cpp
/// @brief provide declarations for DFA

#include "DFA.hpp"
#include "NFA.hpp"
#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"
#include "LexerUtil/Macros.hpp"

#include <boost/dynamic_bitset.hpp>
#include <stack>

using StateSet = boost::dynamic_bitset<>;

DFA::DFA(const NFA &nfa)
    : DFA()
{
    DFA::Powerset(nfa, *this);
}

DFA::DFA()
    : start_(INVALID_STATE_INDEX), states_({}), accepting_({})
{ }


size_t DFA::Start() const
{
    return start_;
}

const std::vector<DFA::State> &DFA::States() const
{
    return states_;
}

const std::unordered_set<size_t> &DFA::Accepting() const
{
    return accepting_;
}


static std::vector<StateSet> InitEpClosureCache(const NFA& nfa)
{
    std::vector<StateSet> closureCache(nfa.states.size());

    for (size_t index = 0; index < nfa.states.size(); ++index )
    {
        StateSet closedList(nfa.states.size()); // set of states already visted
        std::stack<size_t> fringe; // states to visit
        fringe.push(index);

        while (!fringe.empty())
        {   
            size_t index = pop(fringe);
            const NFA::State& state = nfa.states.at(index);
            
            for (const auto& [action, resultState] : state.transitions)
            {
                if (action == EPSILON && !closedList[resultState])
                {
                    closedList.set(resultState);
                    fringe.push(resultState);
                }
            }
        }

        closureCache[index] = std::move(closedList);
    }
    
    return closureCache;
}

static void EpClosure(const std::vector<StateSet>& closureCache, StateSet& set)
{
    for (size_t i = set.find_first(); i != StateSet::npos; i = set.find_next(i))
    {
        set |= closureCache[i];
    }
}

static void Move(const NFA& nfa, char symbol, StateSet& set)
{
    StateSet result(set.size());
    for (size_t i = set.find_first(); i != StateSet::npos; i = set.find_next(i))
    {
        for (const auto& [action, s0] :  nfa.states[i].transitions)
        {
            if (action == symbol)
            {
                result.set(s0);
            }
        }
    }
    set = std::move(result);
}

void DFA::Powerset(const NFA &nfa, DFA &dfa)
{
    std::vector<StateSet> closureCache = InitEpClosureCache(nfa);

    

}
