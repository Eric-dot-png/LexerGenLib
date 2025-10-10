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
#include <queue>

using StateSet = boost::dynamic_bitset<>;
using StateSetHash = boost::hash<boost::dynamic_bitset<>>;

template <typename F>
inline void StateSetIter(const StateSet& set, F&& function)
{
    for (size_t stateIndex = set.find_first(); stateIndex != StateSet::npos;
            stateIndex = set.find_next(stateIndex))
    {
        function(stateIndex);
    }
}

#warning Debug in DFA.cpp
static void Debug(const StateSet& state)
{
    std::string dbgStr = "";
    boost::to_string(state, dbgStr);
    DBG << dbgStr << std::endl;
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

void DFA::Minimize(DFA &dfa)
{
    const size_t N = dfa.states_.size();

    DBG << "Minimizing dfa with " << N << " states." << std::endl;

    /// compute initial partition which contains only the non-accepting 
    /// (therefore non-tagged) set of states in the dfa and not the dead state
    /// as tagged accept states are singular and non-merable, as well as the dead state
    ///
    std::vector<StateSet> partition(1, StateSet(N));
    std::vector<size_t> stateToBlock(N, INVALID_STATE_INDEX);
    for (const State& state : dfa.states_)
    {
        if (state.caseTag == NO_CASE_TAG && state.index != dfa.deadState_)
        {
            partition[0].set(state.index);
            stateToBlock[state.index] = 0;
        }
    }
    
    DBG << "Partition computed: ";
    Debug(partition[0]);

    /// initialize pre map such that the map contains the inverse function delta. 
    /// I.e. pre[c][dest] = set of states which upon c go to dest
    /// 
    std::unordered_map<char, std::unordered_map<size_t, StateSet>> preMap;
    for (char c : ALPHABET)
    {
        preMap[c]; // initialize every character
    }
    for (const State& state : dfa.states_)
    {
        for (const auto& [symbol, result] : state.transitions)
        {
            if (!preMap[symbol].contains(result))
            {
                preMap[symbol][result].resize(N);
            }
            preMap[symbol][result].set(state.index);
        }
    }

    DBG << "PreMap calculated" << std::endl;

    /// initialize work list
    ///
    std::queue<std::pair<StateSet, char>> worklist;
    for (char symbol : ALPHABET)
    {
        worklist.push({partition[0], symbol});
    }

    DBG << "Work list initialized" << std::endl;
 
    /// refine the partition
    ///
    StateSet preSet(N); // set of states that go into a on c
    while(!worklist.empty())
    {
        auto [A, c] = pop(worklist);
        preSet.reset();

        DBG << "Processing ";
        Debug(A);
        DBG << "   on symbol " << Escaped(c) << std::endl;

        if (!A.any()) continue; /// no states to act on (maybe remove?)

        /// compute all the states that have a transition into A from c
        ///
        for (size_t stateI = A.find_first(); stateI != StateSet::npos; stateI = A.find_next(stateI))
        {
            if (preMap[c].contains(stateI))
            {
                DBG << "stateI " << stateI << " : ";
                DBG << "sameSize? " << preSet.size() << ' ' << preMap[c][stateI].size() << std::endl;
                preSet |= preMap[c][stateI]; 
            }
        }

        DBG << "Pre Set: ";
        Debug(preSet);

        for (size_t partitionI = 0; partitionI < partition.size(); ++partitionI)
        {
            StateSet X = partition[partitionI] & preSet;
            StateSet Y = partition[partitionI] & (~preSet);

            if (X.any() && Y.any())
            {
                StateSet& smaller = (X.count() <= Y.count() ? X : Y);
                StateSet& larger = (X.count() > Y.count() ? X : Y);

                partition[partitionI] = larger;
                partition.push_back(smaller);

                /// update the mapping of each state involved
                ///
                for (size_t stateIndex = smaller.find_first(); stateIndex != StateSet::npos; 
                    stateIndex = smaller.find_next(stateIndex))
                {
                    stateToBlock[stateIndex] = partition.size()-1;
                }
                for (size_t stateIndex = larger.find_first(); stateIndex != StateSet::npos; 
                    stateIndex = larger.find_next(stateIndex))
                {
                    stateToBlock[stateIndex] = partitionI;
                }
                
                /// add to the worklist
                ///
                for (char symbol : ALPHABET)
                {
                    worklist.push({smaller, symbol});
                }
            }
        }
    }

    DBG << "Partition refined." << std::endl;

    /// now add the ommited accept states and dead state
    ///
    for (const State& state : dfa.states_)
    {
        if (state.caseTag != NO_CASE_TAG || state.index == dfa.deadState_)
        {
            StateSet singletonSet(N);
            singletonSet.set(state.index);
            partition.push_back(singletonSet);
            stateToBlock[state.index] = partition.size() - 1;
        }
    }

    DBG << "Added omitted states." << std::endl;

    /// finally, make the new set of dfa states
    ///
    std::vector<DFA::State> newStates(partition.size());
    for (size_t partitionI = 0; partitionI < partition.size(); ++partitionI)
    {
        size_t repI = partition[partitionI].find_first();
        DFA::State& repState = dfa.states_[repI];
        newStates[partitionI] = DFA::State{
            .index = partitionI,
            .caseTag = repState.caseTag,
            .transitions = {} // complete this below
        };
        for (const auto& [symbol, oldResult] : repState.transitions)
        {
            newStates[partitionI].transitions[symbol] = stateToBlock[oldResult];
        }
    }
    dfa.states_ = std::move(newStates);
    dfa.start_ = stateToBlock[dfa.start_];
    dfa.deadState_ = stateToBlock[dfa.deadState_];

    DBG << "DFA minimized." << std::endl;
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
    size_t dfaStateRuleTag = NO_CASE_TAG;
    if (accepted.any())
    {
        dfaStateRuleTag = nfa.states[accepted.find_first()].caseTag;
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

    StateSet deadState(nfa.states.size()); /// all 0
    NewState(nfa, nfaAccept, deadState, states, mapping);
    dfa.deadState_ = states.size()-1;
    /// avoid pushing dead state to fringe. DFA stops when encountering dead state,
    /// so no need to calculate anything with dead state

    DBG << "Starting State: ";
    Debug(state);

    DBG << "Dead State: ";
    Debug(deadState);

    /// calculate the powerset construction of nfa 
    ///
    while (!fringe.empty())
    {
        state = pop(fringe);
        StateSet s0;   
        DBG << "Evaluating ";
        Debug(state);

        for (char symbol : ALPHABET)
        {
            s0 = state; 
            Move(nfa, symbol, s0);
            EpClosure(closureCache, s0);
            DBG << std::format("    ({}) resulted in ", Escaped(symbol));
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
