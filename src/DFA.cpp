/// @file DFA.cpp
/// @brief provide declarations for DFA

#include "DFA.hpp"
#include "NFA.hpp"

#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"
#include "LexerUtil/Macros.hpp"

#include <iostream>
#include <string>
#include <boost/dynamic_bitset.hpp>
#include <boost/functional/hash.hpp>
#include <stack>

using StateSet = boost::dynamic_bitset<>;
using StateSetHash = boost::hash<boost::dynamic_bitset<>>;

#warning Debug in DFA.cpp
static void Debug(const StateSet& state)
{
    std::string dbgStr = "";
    boost::to_string(state, dbgStr);
    std::cout << dbgStr << std::endl;
}

DFA::DFA(const NFA &nfa)
    : DFA()
{
    DFA::Powerset(nfa, *this);
}

DFA::DFA()
    : start_(INVALID_STATE_INDEX), states_({}), accepting_({})
{
}

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

static std::vector<StateSet> InitEpClosureCache(const NFA &nfa)
{
    std::vector<StateSet> closureCache(nfa.states.size());

    for (size_t index = 0; index < nfa.states.size(); ++index)
    {
        StateSet closedList(nfa.states.size()); // set of states already visted
        std::stack<size_t> fringe;              // states to visit
        fringe.push(index);

        while (!fringe.empty())
        {
            size_t index = pop(fringe);
            const NFA::State &state = nfa.states.at(index);

            for (const auto &[action, resultState] : state.transitions)
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

static void EpClosure(const std::vector<StateSet> &closureCache, StateSet &set)
{
    for (size_t i = set.find_first(); i != StateSet::npos; i = set.find_next(i))
    {
        set |= closureCache[i];
    }
}

static void Move(const NFA &nfa, char symbol, StateSet &set)
{
    StateSet result(set.size());
    for (size_t i = set.find_first(); i != StateSet::npos; i = set.find_next(i))
    {
        for (const auto &[action, s0] : nfa.states[i].transitions)
        {
            if (action == symbol)
            {
                result.set(s0);
            }
        }
    }
    set = std::move(result);
}

static void NewState(const NFA& nfa, const StateSet& set, std::vector<DFA::State>& states,
    std::unordered_map<StateSet, size_t, StateSetHash>& mapping)
{
    /// determine the state's rule tag (if it has one)
    ///
    size_t dfaStateRuleTag = NO_RULE_TAG;
    for (size_t i = set.find_first(); i != StateSet::npos; i = set.find_next(i))
    {
        if (nfa.states[i].ruleTag != NO_RULE_TAG)
        {
            dfaStateRuleTag = nfa.states[i].ruleTag;
            break;
        }
    }


    /// add the state and update the mapping
    ///
    states.emplace_back(
        states.size(),
        dfaStateRuleTag,
        std::unordered_map<char, size_t>{}
    );

    mapping[set] = states.size()-1;
}

void DFA::Powerset(const NFA &nfa, DFA &dfa)
{
    std::vector<StateSet> closureCache = InitEpClosureCache(nfa);
    
    std::vector<DFA::State>& states = dfa.states_;
    states.reserve(nfa.states.size() / 2); /// heuristically guess max states of dfa
    std::unordered_map<StateSet, size_t, StateSetHash> mapping;
    std::stack<StateSet> fringe;
    
    StateSet state(nfa.states.size()); 
    state.set(nfa.start);
    EpClosure(closureCache, state);
    fringe.push(state);
    NewState(nfa, state, states, mapping);

    std::cout << "Starting State: ";
    Debug(state);

    while (!fringe.empty())
    {
        state = pop(fringe);
        StateSet s0;   
        std::cout << "Evaluating ";
        Debug(state);

        for (char symbol : ALPHABET)
        {
            s0 = state; 
            Move(nfa, symbol, s0);
            EpClosure(closureCache, s0);
            std::cout << std::format("    ({}) resulted in ", Escaped(symbol));
            Debug(s0);

            if (!mapping.contains(s0))
            {
                NewState(nfa, s0, states, mapping);
                fringe.push(s0);
            }
            states[mapping[state]].transitions[symbol] = mapping[s0];
            s0.reset();
        }
    }
}
