/// @file NFABuilder.cpp
/// @brief NFABuilder definitions

#include "NFABuilder.hpp"

#include "NFA.hpp"
#include "RuleCase.hpp"

#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Misc.hpp"

#include <iostream>
#include <ranges>

///
/// Public Methods
/// 

NFA NFABuilder::Build(std::vector<RuleCase> ruleCases)
{
    NFA ret { 
        .start = INVALID_STATE_INDEX,
        .accept = {},
        .states = {},
        .numCases = ruleCases.size()
    };

    std::vector<Fragment> fragments;
    fragments.reserve(ruleCases.size());

    size_t ruleNo = 0;
    size_t startIndex = ret.start = NewState(ret.states, ruleCases.size());

    for (RuleCase& ruleCase : ruleCases)
    {
        PreProcessor::PreProcess(ruleCase);
        Fragment frag{ };
        BuildFragment(ruleCase, ret.states, frag);
        size_t caseIndex = ConcludeCase(ruleNo++, frag, ret.states, ret.accept);
        ret.states[startIndex].transitions.emplace_back(EPSILON, caseIndex);
    }

    return ret;
}

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
                    return MakeCharset(symU.lo, symU.hi, symU.inverted, states);
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
    ENSURES_THROW(false, "Unimplemented");
}

/// @brief In-order implementation of BuildFragment
template <>
auto NFABuilder::BuildFragment<Regex::ItOrder::IN>(const Regex::Flat::Type &expr, 
    std::vector<NFA::State> &states) -> Fragment
{
    ENSURES_THROW(false, "Unimplemented");
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

auto NFABuilder::MakeCharset(char lo, char hi, bool inverted, std::vector<NFA::State> &nfaStates) 
    -> Fragment
{
    // get the range of lo-hi and it's size
    auto range = std::views::iota(lo, hi+1);
    size_t rangeSize = std::ranges::size(range);

    // make the new state and fragment
    size_t q0 = NewState(nfaStates, rangeSize);
    Fragment ret {
        .startIndex = q0,
        .holes = {}
    };
    ret.holes.reserve(rangeSize);

    // fill in the holes
    for (char c : range)
    {
        ret.holes.emplace_back(q0, c);
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
}

auto NFABuilder::ApplyKOpt(const Fragment& fragment,
    std::vector<NFA::State> &nfaStates) -> Fragment
{
    (void)fragment; (void)nfaStates;
    THROW_ERR("Unimplemented '?' operator");
}

auto NFABuilder::ApplyOperator(PreProcessor::Operator_t op, std::stack<Fragment> &fragStack,
    std::vector<NFA::State>& nfaStates) -> Fragment
{
    using enum PreProcessor::Operator_t;
    switch ( op )
    {
    case UNION:
    {
        Fragment right = pop(fragStack);
        Fragment left = pop(fragStack);
        DBG << "Applying Union operator to";
        Debug(left);
        DBG << "and ";
        Debug(right);
        return ApplyUnion(left, right, nfaStates);
    }
    case CONCAT:
    {
        Fragment right = pop(fragStack);
        Fragment left = pop(fragStack);
        DBG << "Applying Concat operator to";
        Debug(left);
        DBG << "and ";
        Debug(right);
        return ApplyCat(left,right,nfaStates);
    }
    case KSTAR:
    {
        Fragment a = pop(fragStack);
        DBG << "Applying KSTAR operator to";
        Debug(a);
        return ApplyKStar(a, nfaStates);
    }
    case KPLUS:
    {
        Fragment a = pop(fragStack);
        DBG << "Applying KPLUS operator to";
        Debug(a);
        return ApplyKPlus(a, nfaStates);
    }
    case OPTIONAL:
    {
        Fragment a = pop(fragStack);
        DBG << "Applying OPTIONAL operator to";
        Debug(a);
        return ApplyKOpt(a, nfaStates);
    }
    default: THROW_ERR("Unhandled case in NFABuilder::ApplyOperator()");
    }
    UNREACHABLE();
}

void NFABuilder::BuildFragment(const RuleCase &pattern,  
    std::vector<NFA::State>& nfaStates, Fragment &fragment)
{
    /// use shunting yard if the pattern is a regex
    /// 
    if (pattern.patternType == RuleCase::Pattern_t::REGEX) 
    {
        ShuntingYard(pattern, fragment, nfaStates);
        return;
    }

    /// skip processing none pattern and eof pattern
    ///
    else if (pattern.patternType == RuleCase::Pattern_t::NONE || 
        pattern.patternType == RuleCase::Pattern_t::END_OF_FILE)
    {
        return;
    }

    /// use standard literal interpretation if the pattern is a string
    ///
    std::string_view literalView = pattern.patternData;
    fragment = MakeChar(literalView[0], nfaStates);
    for (size_t i = 1; i <literalView.size();++i)
    {
        Fragment right = MakeChar(literalView[i], nfaStates);
        fragment = ApplyCat(fragment,right,nfaStates);
    }
}

size_t NFABuilder::ConcludeCase(size_t ruleNo, Fragment &ruleFragment, std::vector<NFA::State> &nfaStates, 
    std::unordered_set<size_t> &nfaAccepting)
{
    size_t acceptState = NewState(nfaStates, ruleNo, 1);
    PatchHoles(ruleFragment.holes, acceptState, nfaStates);
    nfaAccepting.insert(acceptState);
    return ruleFragment.startIndex;
}

#include <iomanip>

void NFABuilder::ShuntingYard(const RuleCase &ruleCase, Fragment &fragment,
    std::vector<NFA::State>& nfaStates)
{
    bool expectOperand = true; /// true if we expect an operand next, false if we expect an operator next
    std::stack<PreProcessor::Operator_t> opStack; /// stack to hold operators
    std::stack<Fragment> fragStack; /// stack to hold fragments
    std::string_view pattern = ruleCase.patternData;
    
    /// iterate over the pattern, and convert to RPN using the shunting-yard algorithm
    ///
    for (char c : pattern) 
    {
        DBG << "ShuntingYard pass: 0x" << std::setfill('0')  
            << std::hex << (int)c << std::setfill(' ') << std::dec << std::endl; 

        if (!PreProcessor::IsOperator(c))
        {
            EXPECTS_THROW(expectOperand, std::format("Expected literal, got '{}'", c));
            fragStack.push(MakeChar(c, nfaStates));
            DBG << "Pushed Literal Fragment ";
            Debug(fragStack.top());
            expectOperand = false;
        }
        else
        {
            PreProcessor::Operator_t op = PreProcessor::OperatorOf(c);
            switch(op)
            {
            case PreProcessor::Operator_t::LPAREN: 
            {
                DBG << "Found LPAREN" << std::endl;
                opStack.push(PreProcessor::Operator_t::LPAREN);
                expectOperand = true;
                break;
            }
            case PreProcessor::Operator_t::RPAREN:
            {
                DBG << "Found RPAREN" << std::endl;
                EXPECTS_THROW(!expectOperand, "TODO: Unkerr?");

                /// pop off the stack until we find a left paren
                /// if we run out of stack before finding a left paren, then we have mismatch
                ///
                while(!opStack.empty() && opStack.top() != PreProcessor::Operator_t::LPAREN) 
                {
                    PreProcessor::Operator_t op = pop(opStack);
                    fragStack.push(ApplyOperator(op, fragStack, nfaStates));
                    DBG << "Pushed ";
                    Debug(fragStack.top());
                }
                ENSURES_THROW(!opStack.empty() && opStack.top() == PreProcessor::Operator_t::LPAREN, "TODO: UNKERR?");
                
                opStack.pop(); // remove the lparen
                expectOperand = false; 
                break;
            }
            default:
            {
                DBG << "Found an operator" << std::endl;
                EXPECTS_THROW(!expectOperand, "Unexpected operator");

                /// pop off the stack until we find a lower precedence operator or a left paren or empty
                ///    
                while (!opStack.empty() && opStack.top() != PreProcessor::Operator_t::LPAREN &&
                        (PreProcessor::PriorityOf(opStack.top()) > PreProcessor::PriorityOf(op) || 
                         (PreProcessor::PriorityOf(opStack.top()) == PreProcessor::PriorityOf(op) 
                            && PreProcessor::isBinary(op)) ) )
                {
                    PreProcessor::Operator_t op2 = pop(opStack);
                    fragStack.push(ApplyOperator(op2, fragStack, nfaStates));
                    DBG << "Pushed ";
                    Debug(fragStack.top());
                }

                opStack.push(op);
                expectOperand = !PreProcessor::isBinary(op);
            }
            }
        }
    }

    /// process the rest of the operator stack
    ///
    while (!opStack.empty())
    {
        PreProcessor::Operator_t op = pop(opStack);
        ENSURES_THROW(op != PreProcessor::Operator_t::LPAREN && op != PreProcessor::Operator_t::RPAREN,  "TODO: UNKERR?"); 
        
        fragStack.push(ApplyOperator(op, fragStack, nfaStates));
    }
    ENSURES_THROW(fragStack.size() == 1, "TODO: UNKERR?");

    fragment = std::move(fragStack.top());
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

