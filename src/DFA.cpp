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
    : start_(INVALID_STATE_INDEX), deadState_(INVALID_STATE_INDEX), states_({})
{ }

size_t DFA::Start() const
{
    return start_;
}

size_t DFA::Dead() const
{
    return deadState_;
}

const std::vector<DFA::State> &DFA::States() const
{
    return states_;
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

static void NewState(const NFA& nfa, const StateSet& nfaAccepting, const StateSet& nfaStateSet, 
    std::vector<DFA::State>& states, std::unordered_map<StateSet, size_t, StateSetHash>& mapping)
{
    /// calculate set of accepting states in the set of states and use first available rule tag
    ///
    StateSet accepted = (nfaStateSet & nfaAccepting);
    size_t dfaStateRuleTag = NO_RULE_TAG;
    if (accepted.any())
    {
        dfaStateRuleTag = nfa.states[accepted.find_first()].ruleTag;
    }

    /// add the state and update the mapping of the state set to the state index
    ///
    states.emplace_back(
        states.size(),
        dfaStateRuleTag,
        std::unordered_map<char, size_t>{}
    );
    mapping[nfaStateSet] = states.size()-1;
}

void DFA::Powerset(const NFA &nfa, DFA &dfa)
{
    /// initialize cache of nfa closures and bitset for nfa accepting state
    ///
    std::vector<StateSet> closureCache = InitEpClosureCache(nfa);
    StateSet nfaAccept(nfa.states.size());
    for (size_t astate : nfa.accept)
    {
        nfaAccept.set(astate);
    }

    /// setyp dfa related variables
    ///
    std::vector<DFA::State>& states = dfa.states_;
    states.reserve(nfa.states.size() / 2); /// heuristically guess max states of dfa
    std::unordered_map<StateSet, size_t, StateSetHash> mapping;
    
    /// initialize fringe and add starting and dead state to it
    ///
    std::stack<StateSet> fringe;
    
    StateSet state(nfa.states.size()); 
    state.set(nfa.start);
    EpClosure(closureCache, state);
    fringe.push(state);
    NewState(nfa, nfaAccept, state, states, mapping);

    StateSet deadState(nfa.states.size());
    NewState(nfa, nfaAccept, deadState, states, mapping);
    dfa.deadState_ = states.size()-1;

    std::cout << "Starting State: ";
    Debug(state);

    std::cout << "Dead State: ";
    Debug(deadState);

    /// calculate the powerset construction of nfa 
    ///
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
                NewState(nfa, nfaAccept, s0, states, mapping);
                fringe.push(s0);
            }
            states[mapping[state]].transitions[symbol] = mapping[s0];
            s0.reset();
        }
    }
}
