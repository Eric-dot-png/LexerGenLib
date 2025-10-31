/// @file NFABuilder.cpp
/// @brief NFABuilder definitions

#include "NFABuilder.hpp"

#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Misc.hpp"

#include "NFA.hpp"

#include <iostream>
#include <ranges>

///
/// Public Methods
/// 

/// @todo move internal logic to worker function, and pass the instance of build arg to the
///       worker method - reduce binary size
template <Regex::ItOrder it>
NFA NFABuilder::Build(const std::vector<Regex::Flat::Type>& exprs)
{
    NFA ret {
        .start = INVALID_STATE_INDEX,
        .accept = {},
        .states = {},
        .numCases = exprs.size()
    };

    ret.start = NewState(ret.states, exprs.size());

    std::vector<Fragment> frags; frags.reserve(exprs.size());
    for (const auto& [ruleNo, expr] : std::views::enumerate(exprs))
    {
        Fragment ruleFrag = BuildFragment<it>(expr, ret.states);
        size_t caseIndex = ConcludeCase(ruleNo, ruleFrag, ret.states, ret.accept);
        ret.states[ret.start].transitions.emplace_back(EPSILON, caseIndex);
    }

    return ret;
}

template NFA NFABuilder::Build<Regex::ItOrder::PRE>(const std::vector<Regex::Flat::Type>&);
template NFA NFABuilder::Build<Regex::ItOrder::IN>(const std::vector<Regex::Flat::Type>&);
template NFA NFABuilder::Build<Regex::ItOrder::POST>(const std::vector<Regex::Flat::Type>&);

/// -----------------------------------------------------------------------------------------------
/// Private Methods
/// -----------------------------------------------------------------------------------------------


/// -----------------------------------------------------------------------------------------------
/// Build Fragments Methods
/// -----------------------------------------------------------------------------------------------

/// @brief Post-order implementation of BuildFragment
template <>
auto NFABuilder::BuildFragment<Regex::ItOrder::POST>(const Regex::Flat::Type &expr, 
    std::vector<NFA::State> &states) -> Fragment
{
    using namespace Regex::Flat;
    
    std::stack<Fragment> fragments;

    for (const Symbol& sym : expr)
    {
        fragments.push(
            std::visit([&](auto&& symU) -> Fragment
            {   
                /// get the underlying type of symU
                using T = std::decay_t<decltype(symU)>;

                /// act on terminal types
                if constexpr (std::is_same_v<T, Char_t>)
                {
                    return MakeChar(symU.value, states);
                }
                else if constexpr (std::is_same_v<T, Charset_t>)
                {
                    return MakeCharset(symU.chars, symU.inverted, states);
                }
                else if constexpr (std::is_same_v<T, Literal_t>)
                {
                    return MakeLiteral(symU.value, states);
                }

                /// act on non-terminal operator types
                else if constexpr (std::is_same_v<T, Union_t>)
                {
                    Fragment right = pop(fragments);
                    Fragment left = pop(fragments);
                    return ApplyUnion(left, right, states);
                }
                else if constexpr (std::is_same_v<T, Concat_t>)
                {
                    Fragment right = pop(fragments);
                    Fragment left = pop(fragments);
                    return ApplyCat(left, right, states);
                }
                else if constexpr (std::is_same_v<T, KleeneStar_t>)
                {
                    Fragment frag = pop(fragments);
                    return ApplyKStar(frag, states);
                }
            }, sym)
        );
    }
    ENSURES_THROW(fragments.size() == 1, "Unexpected additional fragments in postorder evaluation");
    return fragments.top();
}

/// @brief Pre-order implementation of BuildFragment
template <>
auto NFABuilder::BuildFragment<Regex::ItOrder::PRE>(const Regex::Flat::Type &expr, 
    std::vector<NFA::State> &states) -> Fragment
{
    (void) expr; (void)states;
    ENSURES_THROW(false, "Unimplemented");
    return Fragment{ };
}

/// @brief In-order implementation of BuildFragment
template <>
auto NFABuilder::BuildFragment<Regex::ItOrder::IN>(const Regex::Flat::Type &expr, 
    std::vector<NFA::State> &states) -> Fragment
{
    (void) expr; (void)states;
    ENSURES_THROW(false, "Unimplemented");
    return Fragment{ };
}

/// -----------------------------------------------------------------------------------------------
/// New state / fragment management methods
/// -----------------------------------------------------------------------------------------------

size_t NFABuilder::NewState(std::vector<NFA::State> &nfaStates, size_t caseNo, size_t estTCount)
{
    size_t stateIndex = nfaStates.size();
    nfaStates.emplace_back(stateIndex, caseNo, std::vector<NFA::Transition>{});
    nfaStates.back().transitions.reserve(estTCount);
    return stateIndex;
}

size_t NFABuilder::NewState(std::vector<NFA::State>& nfaStates, size_t estTCount)
{
    return NewState(nfaStates, NO_CASE_TAG, estTCount);
}

void NFABuilder::PatchHoles(const std::vector<Fragment::Hole> &holes, 
    size_t patchState, std::vector<NFA::State> &nfaStates)
{
    DBG << "PatchHoles(holes, " << patchState << ")\n";
    for (const auto& hole : holes)
    {
        DBG << "    " << hole.holeIndex << "['" << hole.tVal << "'] = " << patchState << "\n"; 
        nfaStates[hole.holeIndex].transitions.emplace_back(hole.tVal, patchState);
    }
}

auto NFABuilder::MakeChar(char a, std::vector<NFA::State> &nfaStates)
    -> NFABuilder::Fragment
{
    size_t q0 = NewState(nfaStates, 1);
    
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

auto NFABuilder::MakeCharset(const std::unordered_set<char>& chars, bool inverted, std::vector<NFA::State> &nfaStates) 
    -> Fragment
{
    size_t size = (inverted ? ALPHABET.size() - chars.size() : chars.size());


    // make the new state and fragment
    size_t q0 = NewState(nfaStates, size);
    Fragment ret {
        .startIndex = q0,
        .holes = {}
    };
    ret.holes.reserve(size);

    // fill in the fragment holes
    if (inverted)
    {
        for (char c : ALPHABET)
        {
            if (!chars.contains(c))
            {
                ret.holes.emplace_back(q0, c);
            }
        }
    }
    else
    {
        for (char c : chars)
        {
            ret.holes.emplace_back(q0, c);
        }
    }

    // return range fragment
    return ret;
}

auto NFABuilder::MakeLiteral(std::string_view string, std::vector<NFA::State> &nfaStates) 
    -> Fragment
{
    /// make sure the string isnt "" (doesnt make sense)
    EXPECTS_THROW(string.size() > 0, "Requested Literal is empty");

    /// initialize 
    size_t index = 0;
    Fragment first = MakeChar(string[index], nfaStates);
    Fragment curr{ };

    /// perform concatination over the literal
    for (++index; index < string.size(); ++index)
    {
        curr = MakeChar(string[index], nfaStates);
        first = ApplyCat(first, curr, nfaStates);
    }

    /// return the first fragment (which we continously applied concat to)
    return first;
}

auto NFABuilder::ApplyCat(const Fragment &left, const Fragment &right,
    std::vector<NFA::State>& nfaStates) -> Fragment
{
    PatchHoles(left.holes, right.startIndex, nfaStates);    
    return Fragment{left.startIndex, right.holes };
}

auto NFABuilder::ApplyUnion(const Fragment &left, const Fragment &right,
    std::vector<NFA::State>& nfaStates) -> Fragment
{
    /// add a new state and perform the union on the fragments
    ///
    size_t newStateIndex = NewState(nfaStates, 2);
    nfaStates[newStateIndex].transitions = {
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
    ret.holes.insert(ret.holes.end(), right.holes.begin(), right.holes.end());
    
    return ret;
}

auto NFABuilder::ApplyKStar(const Fragment &fragment, 
    std::vector<NFA::State> &nfaStates) -> Fragment
{
    size_t newStateIndex = NewState(nfaStates, 2);

    /// add new epsilon transition to new state to advance (without consuming)
    /// a symbol
    ///
    nfaStates[newStateIndex].transitions = {
        NFA::Transition{
            .symbol = EPSILON,
            .to = fragment.startIndex
        }
    };

    /// patch the holes of the fragment
    ///
    PatchHoles(fragment.holes, newStateIndex, nfaStates);
    
    return Fragment{
        .startIndex = newStateIndex,
        .holes = {
            Fragment::Hole{
                .holeIndex = fragment.startIndex,
                .tVal = EPSILON
            }
        } 
    };
}

auto NFABuilder::ApplyKPlus(const Fragment &fragment, 
    std::vector<NFA::State> &nfaStates) -> Fragment
{
    (void)fragment; (void)nfaStates;
    THROW_ERR("Unimplemented '+' operator");
    return Fragment{ };
}

auto NFABuilder::ApplyKOpt(const Fragment& fragment,
    std::vector<NFA::State> &nfaStates) -> Fragment
{
    (void)fragment; (void)nfaStates;
    THROW_ERR("Unimplemented '?' operator");
    return Fragment{ };
}

size_t NFABuilder::ConcludeCase(size_t ruleNo, Fragment &ruleFragment, std::vector<NFA::State> &nfaStates, 
    std::unordered_set<size_t> &nfaAccepting)
{
    size_t acceptState = NewState(nfaStates, ruleNo, 1);
    PatchHoles(ruleFragment.holes, acceptState, nfaStates);
    nfaAccepting.insert(acceptState);
    return ruleFragment.startIndex;
}

/// -----------------------------------------------------------------------------------------------
/// Debug methods
/// -----------------------------------------------------------------------------------------------

void NFABuilder::Debug(const Fragment &frag)
{
    DBG << "<Fragment " << &frag << ", startIndex=" << frag.startIndex
        << ", holes=[";
    for (const Fragment::Hole& hole : frag.holes)
    {
        DBG << '(' << hole.holeIndex << ", \'"
            << hole.tVal << "\') ";
    }
    DBG << "]>" << std::endl;
}

