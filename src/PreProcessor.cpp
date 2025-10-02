/// @file RegexBuilder.cpp
/// @brief Implementation of RegexBuilder

#include "PreProcessor.hpp"

#include <iostream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <string_view>

#include "RuleCase.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"

#warning dbg in PreProcessor.cpp
static void dbg(RuleCase& ruleCase)
{
    std::cout << std::hex << std::setfill('0');
    for (char c : ruleCase.patternData)
    {
        std::cout << "0x" << std::setw(2) << (int) c << ' ';
    }
    std::cout << std::endl;
}

void PreProcessor::PreProcess(RuleCase &ruleCase)
{
    std::string& pattern = ruleCase.patternData;

    switch( ruleCase.patternType )
    {
    case RuleCase::Pattern_t::END_OF_FILE:
    case RuleCase::Pattern_t::NONE:
    {
        pattern = "";
        return;
    }
    case RuleCase::Pattern_t::STRING:
    {
        return;
    }
    case RuleCase::Pattern_t::REGEX:
    {
        break;
    }
    }

    Encode(pattern);

    UnifyRanges(pattern);

    InsertConcats(pattern);
}

void PreProcessor::PreProcess(std::vector<RuleCase> &patterns)
{
    for (RuleCase& pattern : patterns)
    {
        PreProcess(pattern);
    }
}

bool PreProcessor::IsOperator(char c)
{
    char decoded = Decode(c);
    return (decoded != c);
}

auto PreProcessor::OperatorOf(char c) -> Operator_t
{
    ENSURES_THROW(IsOperator(c), std::format("Invalid operator '{}' requested", c));
    
    switch((OpEncoded) c)
    {
        case OpEncoded::UNION: return Operator_t::UNION;
        case OpEncoded::CONCAT: return Operator_t::CONCAT;
        case OpEncoded::KLEENE: return Operator_t::KSTAR;
        case OpEncoded::PLUS: return Operator_t::KPLUS;
        case OpEncoded::OPTIONAL: return Operator_t::OPTIONAL;
        case OpEncoded::LPAREN: return Operator_t::LPAREN;
        case OpEncoded::RPAREN: return Operator_t::RPAREN;
        default: break;
    }
    
    throw( std::logic_error("unreachable") );
}

uint32_t PreProcessor::PriorityOf(Operator_t op)
{
    switch( op )
    {
    case Operator_t::UNION: return 0;
    case Operator_t::CONCAT: return 1;
    
    case Operator_t::KSTAR:
    case Operator_t::KPLUS: 
    case Operator_t::OPTIONAL: 
    {
        return 2;
    }
    case Operator_t::LPAREN: return 3;
    case Operator_t::RPAREN: return 4;
    default: return -1;
    }
}

bool PreProcessor::isBinary(Operator_t op)
{
    switch( op )
    {
    case Operator_t::KSTAR:
    case Operator_t::KPLUS:
    case Operator_t::OPTIONAL:
    {
        return true;
    }    
    default: 
    {
        return false;
    }
    }
}

template <typename Operator_t>
auto PreProcessor::GetType(char c) -> SymbolClass
{  
    switch(( (Operator_t) c))
    {
    case Operator_t::UNION:
    case Operator_t::CONCAT:
        return SymbolClass::BINARY_OP;

    case Operator_t::KLEENE:
    case Operator_t::PLUS:
    case Operator_t::OPTIONAL:
        return SymbolClass::UNARY_OP;

    case Operator_t::LPAREN: return SymbolClass::LPAREN;
    case Operator_t::RPAREN: return SymbolClass::RPAREN;
    
    case Operator_t::LBRACE:
    case Operator_t::RBRACE:
    case Operator_t::INVERT:
    case Operator_t::RANGE_MID:
        return SymbolClass::RANGE_OP;
    
    default: return SymbolClass::LITERAL; // if not an operator, then it's a literal
    }
}

template PreProcessor::SymbolClass PreProcessor::GetType<PreProcessor::OpDecoded>(char c);
template PreProcessor::SymbolClass PreProcessor::GetType<PreProcessor::OpEncoded>(char c);

auto PreProcessor::EncodeOp(OpDecoded op) -> OpEncoded
{
    switch(op) 
    {
    case OpDecoded::UNION:     return OpEncoded::UNION;
    case OpDecoded::CONCAT:    return OpEncoded::CONCAT;
    case OpDecoded::KLEENE:    return OpEncoded::KLEENE;
    case OpDecoded::PLUS:      return OpEncoded::PLUS;
    case OpDecoded::OPTIONAL:  return OpEncoded::OPTIONAL;
    case OpDecoded::LPAREN:    return OpEncoded::LPAREN;
    case OpDecoded::RPAREN:    return OpEncoded::RPAREN;
    case OpDecoded::LBRACE:    return OpEncoded::LBRACE;
    case OpDecoded::RBRACE:    return OpEncoded::RBRACE;
    case OpDecoded::INVERT:    return OpEncoded::INVERT;
    case OpDecoded::RANGE_MID: return OpEncoded::RANGE_MID;
    default: EXPECTS_THROW(false, "TODO: UNKERR?");
    }
}

char PreProcessor::Decode(char c)
{
    switch( (OpEncoded) c )
    {
        case OpEncoded::UNION:     return (char)OpDecoded::UNION;
        case OpEncoded::CONCAT:    return (char)OpDecoded::CONCAT;
        case OpEncoded::KLEENE:    return (char)OpDecoded::KLEENE;
        case OpEncoded::PLUS:      return (char)OpDecoded::PLUS;
        case OpEncoded::OPTIONAL:  return (char)OpDecoded::OPTIONAL;
        case OpEncoded::LPAREN:    return (char)OpDecoded::LPAREN;
        case OpEncoded::RPAREN:    return (char)OpDecoded::RPAREN;
        case OpEncoded::LBRACE:    return (char)OpDecoded::LBRACE;
        case OpEncoded::RBRACE:    return (char)OpDecoded::RBRACE;
        case OpEncoded::INVERT:    return (char)OpDecoded::INVERT;
        case OpEncoded::RANGE_MID: return (char)OpDecoded::RANGE_MID;
        default: return c;
    }
}

void PreProcessor::Encode(std::string &pattern)
{
    std::string ret;
    ret.reserve(pattern.size());

    for (size_t i=0;i<pattern.size();++i) 
    {
        if (pattern[i] == '\\') 
        {
            i += 1; // advance to skip the next char (which is escaped)
            ENSURES_THROW(i < pattern.size(), "Unmatched '\'."); // unmatched '\'
            ret.push_back(pattern[i]); // add the escaped char as a literal
        }
        else if (GetType<OpDecoded>(pattern[i]) != SymbolClass::LITERAL) 
        {
            ret.push_back((char) EncodeOp((OpDecoded) pattern[i]));
        }
        else 
        {
            ret.push_back(pattern[i]);
        }
    }

    pattern = std::move(ret);   
}

void PreProcessor::UnifyRanges(std::string &pattern)
{
    std::stringstream ss; // output string stream to set the pattern to
    size_t startI = 0; // starting index of the current range
    size_t endI = 0; // ending index of the current range

    while(1)
    {
        bool invertedRange = false;
        /// find the first occurence of an encoded left range op in the string
        ///
        startI = pattern.find((char)OpEncoded::LBRACE, endI);
       
        /// include all the characters outside of the string in the output string
        ///
        for (size_t i = endI; i < (startI == std::string::npos ? pattern.size() : startI);++i) 
        {
            ss << pattern[i];
        }

        /// handle right range operator found without left range op e.g. "123]"
        /// 
        endI = pattern.find((char)OpEncoded::RBRACE, (startI == std::string::npos ? endI : startI));
        ENSURES_THROW( !(startI == std::string::npos && endI != std::string::npos), 
            std::format("Unmatched {} in regex \"{}\"", (char)OpDecoded::RBRACE, RegexStr(pattern)));

        if (startI == std::string::npos) break; /// there is no more range 
        ENSURES_THROW(endI != std::string::npos, std::format("Unmatched {}, in regex \"{}\"", (char)OpDecoded::LBRACE, RegexStr(pattern)));

        /// determine if the range is inverted, and actually form the range
        ///
        invertedRange = (pattern[startI+1] == (char)OpEncoded::INVERT); 
        startI = (invertedRange ? startI+2 : startI+1);
        std::string_view range = pattern.substr(startI, endI - startI);    
        ENSURES_THROW(range.size() > 0, std::format("Empty {}{} in regex \"{}\"", 
            (char)OpDecoded::LBRACE, (char)OpDecoded::RBRACE, RegexStr(pattern)));
        
        /// finally, actually calculate this specific range instance
        ///
        std::unordered_set<char> rangeSet = { };
        std::unordered_set<char> invRangeSet = ALPHABET;
        for (size_t rangeI = 0; rangeI < range.size(); ++rangeI)
        {
            if (rangeI + 2 < range.size() && range[rangeI+1] == (char) OpEncoded::RANGE_MID)
            {
                for (char c = Decode(range[rangeI]); c <= Decode(range[rangeI+2]);++c)
                {
                    rangeSet.insert(c);
                    invRangeSet.erase(c);
                }
                rangeI += 2;
            }
            else 
            {
                char decodedC = Decode(range[rangeI]);
                rangeSet.insert(decodedC);
                invRangeSet.erase(decodedC);
            }
        }
        ss << (char)OpEncoded::LPAREN;
        std::string_view delim = "";
        for (char c : (invertedRange ? invRangeSet : rangeSet))
        {
            ss << delim << c;
            delim = std::string(1, (char)OpEncoded::UNION);
        }
        ss << (char)OpEncoded::RPAREN;
        
        ++endI; // advance the end so we don't see the same rbrace
    }
    pattern = std::move(ss.str());
}

void PreProcessor::InsertConcats(std::string &pattern)
{
    std::stringstream ss;

    ss << pattern[0];
    for (size_t i = 1; i < pattern.size();++i)
    {
        SymbolClass left = GetType<OpEncoded>(pattern[i-1]);
        SymbolClass right = GetType<OpEncoded>(pattern[i]);
        if ( (left == SymbolClass::LITERAL || left == SymbolClass::UNARY_OP || left == SymbolClass::RPAREN) &&
             (right == SymbolClass::LITERAL || right == SymbolClass::LPAREN) )
        {
            ss << (char)OpEncoded::CONCAT;
        }
        ss << pattern[i];
    }
    pattern = std::move(ss.str());
}

void PreProcessor::makeRPN(std::string &pattern)
{
    /// define the operator precedence map
    ///
    static const std::unordered_map<OpEncoded, uint32_t> OP_PRIO = {
        { OpEncoded::UNION, 1 },
        { OpEncoded::CONCAT, 2 },
        { OpEncoded::KLEENE, 3 },
        { OpEncoded::PLUS, 3 },
        { OpEncoded::OPTIONAL, 3 }
    };

    bool expectOperand = true; /// true if we expect an operand next, false if we expect an operator next
    std::stack<OpEncoded> opStack; /// stack to hold operators
    std::string ret; /// the resulting RPN string (to be moved)
    ret.reserve(pattern.size()); /// reserve space to avoid multiple allocations

    /// iterate over the pattern, and convert to RPN using the shunting-yard algorithm
    ///
    for (char c : pattern) 
    {
        SymbolClass type = GetType<OpEncoded>(c);
        switch(type)
        {
        case SymbolClass::LITERAL:
{
            EXPECTS_THROW(expectOperand, "TODO: UNKERR?");
            ret.push_back(c);
            expectOperand = false;
            break;
        }
        case SymbolClass::LPAREN:
        {
            opStack.push(OpEncoded::LPAREN);
            expectOperand = true;
            break;
        }
        case SymbolClass::RPAREN:
        {
            EXPECTS_THROW(!expectOperand, "TODO: UNKERR?");
            
            /// pop off the stack until we find a left paren
            /// if we run out of stack before finding a left paren, then we have mismatch
            ///
            while(!opStack.empty() && opStack.top() != OpEncoded::LPAREN) 
            {
                ret.push_back((char)pop(opStack));
            }
            ENSURES_THROW(opStack.empty() || opStack.top() != OpEncoded::LPAREN, "TODO: UNKERR?");
            
            opStack.pop(); // remove the lparen
            expectOperand = false; 
            break;
        }
        case SymbolClass::BINARY_OP:
        case SymbolClass::UNARY_OP:
        {
            EXPECTS_THROW(!expectOperand, "TODO: UNKERR?");

            /// pop off the stack until we find a lower precedence operator or a left paren
            ///
            while(!opStack.empty() && opStack.top() != OpEncoded::LPAREN &&
                  ((OP_PRIO.at(opStack.top()) > OP_PRIO.at((OpEncoded) c)) ||
                  (OP_PRIO.at(opStack.top()) == OP_PRIO.at((OpEncoded) c) && type == SymbolClass::BINARY_OP))) 
            {
                ret.push_back((char)pop(opStack));
            }

            opStack.push((OpEncoded) c); // push the current operator onto the stack
            expectOperand = (type == SymbolClass::BINARY_OP); // binary operator requries next symbol to be oprnd
            break;
        }
        default: EXPECTS_THROW(false, "TODO: UNKERR?");
        }
    }

    /// process the rest of the operator stack
    ///
    while (!opStack.empty())
    {
        OpEncoded op = opStack.top();
        ENSURES_THROW(op != OpEncoded::LPAREN && op != OpEncoded::RPAREN,  "TODO: UNKERR?"); 
        ret += (char)op;
        opStack.pop();
    }

    pattern = std::move(ret);
}

void PreProcessor::PrintRegex(std::ostream &os, const std::string &pattern)
{
    for (char c : pattern)
    {
        os << Decode(c);
    }
}

std::string PreProcessor::RegexStr(const std::string &pattern)
{
    std::stringstream ss;
    PrintRegex(ss, pattern);
    return ss.str();
}