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

#include "Pattern.hpp"
#include "LexerUtil/Macros.hpp"
#include "LexerUtil/Constants.hpp"
#include "LexerUtil/Misc.hpp"

static void dbg(Pattern& patt)
{
    std::cout << std::hex << std::setfill('0');
    for (char c : patt.pattern)
    {
        std::cout << "0x" << std::setw(2) << (int) c << ' ';
    }
    std::cout << std::endl;
}

void PreProcessor::PreProcess(Pattern &pattern)
{
    if (!pattern.isRegex) return; // the pattern is a literal, no processing needed
    std::cout << RegexStr(pattern) << '\n';
    
    Encode(pattern);
    std::cout << RegexStr(pattern) << '\n';

    dbg(pattern);

    UnifyRanges(pattern);
    std::cout << RegexStr(pattern) << '\n';

    InsertConcats(pattern);
    std::cout << RegexStr(pattern) << '\n';
}

void PreProcessor::PreProcess(std::vector<Pattern> &patterns)
{
    for (Pattern& pattern : patterns)
    {
        PreProcess(pattern);
    }
}

template <typename Operator_t>
auto PreProcessor::GetType(char c) -> CharType
{  
    switch(( (Operator_t) c))
    {
    case Operator_t::UNION:
    case Operator_t::CONCAT:
        return CharType::BINARY_OP;

    case Operator_t::KLEENE:
    case Operator_t::PLUS:
    case Operator_t::OPTIONAL:
        return CharType::UNARY_OP;

    case Operator_t::LPAREN: return CharType::LPAREN;
    case Operator_t::RPAREN: return CharType::RPAREN;
    
    case Operator_t::LBRACE:
    case Operator_t::RBRACE:
    case Operator_t::INVERT:
    case Operator_t::RANGE_MID:
        return CharType::RANGE_OP;
    
    default: return CharType::LITERAL; // if not an operator, then it's a literal
    }
}

template PreProcessor::CharType PreProcessor::GetType<PreProcessor::OpDecoded>(char c);
template PreProcessor::CharType PreProcessor::GetType<PreProcessor::OpEncoded>(char c);

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

void PreProcessor::Encode(Pattern &pattern)
{
    std::string ret;
    ret.reserve(pattern.pattern.size());

    for (size_t i=0;i<pattern.pattern.size();++i) 
    {
        if (pattern.pattern[i] == '\\') 
        {
            i += 1; // advance to skip the next char (which is escaped)
            ENSURES_THROW(i < pattern.pattern.size(), "Unmatched '\'."); // unmatched '\'
            ret.push_back(pattern.pattern[i]); // add the escaped char as a literal
        }
        else if (GetType<OpDecoded>(pattern.pattern[i]) != CharType::LITERAL) 
        {
            ret.push_back((char) EncodeOp((OpDecoded) pattern.pattern[i]));
        }
        else 
        {
            ret.push_back(pattern.pattern[i]);
        }
    }

    pattern.pattern = std::move(ret);   
}

void PreProcessor::UnifyRanges(Pattern &pattern)
{
    std::stringstream ss; // output string stream to set the pattern to
    std::string_view regx = pattern.pattern; // view of the regex pattern
    size_t startI = 0; // starting index of the current range
    size_t endI = 0; // ending index of the current range

    while(1)
    {
        bool invertedRange = false;
        /// find the first occurence of an encoded left range op in the string
        ///
        startI = regx.find((char)OpEncoded::LBRACE, endI);
       
        /// include all the characters outside of the string in the output string
        ///
        for (size_t i = endI; i < (startI == std::string::npos ? regx.size() : startI);++i) 
        {
            ss << regx[i];
        }

        /// handle right range operator found without left range op e.g. "123]"
        /// 
        endI = regx.find((char)OpEncoded::RBRACE, (startI == std::string::npos ? endI : startI));
        ENSURES_THROW( !(startI == std::string::npos && endI != std::string::npos), 
            std::format("Unmatched {} in regex \"{}\"", (char)OpDecoded::RBRACE, RegexStr(pattern)));

        if (startI == std::string::npos) break; /// there is no more range 
        ENSURES_THROW(endI != std::string::npos, std::format("Unmatched {}, in regex \"{}\"", (char)OpDecoded::LBRACE, RegexStr(pattern)));

        /// determine if the range is inverted, and actually form the range
        ///
        invertedRange = (regx[startI+1] == (char)OpEncoded::INVERT); 
        startI = (invertedRange ? startI+2 : startI+1);
        std::string_view range = regx.substr(startI, endI - startI);    
        std::cout << "found range: ";
        PrintRegex(std::cout, {std::string(range), true});
        std::cout << std::endl;
        ENSURES_THROW(range.size() > 0, std::format("Empty {}{} in regex \"{}\"", (char)OpDecoded::LBRACE, (char)OpDecoded::RBRACE, RegexStr(pattern)));
        
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
    pattern.pattern = std::move(ss.str());
}

void PreProcessor::InsertConcats(Pattern &pattern)
{
    std::stringstream ss;
    std::string_view regx = pattern.pattern;

    ss << regx[0];
    for (size_t i = 1; i < regx.size();++i)
    {
        CharType left = GetType<OpEncoded>(regx[i-1]);
        CharType right = GetType<OpEncoded>(regx[i]);
        if ( (left == CharType::LITERAL || left == CharType::UNARY_OP || left == CharType::RPAREN) &&
             (right == CharType::LITERAL || right == CharType::LPAREN) )
        {
            ss << (char)OpEncoded::CONCAT;
        }
        ss << regx[i];
    }
    pattern.pattern = std::move(ss.str());
}

void PreProcessor::makeRPN(Pattern &pattern)
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
    ret.reserve(pattern.pattern.size()); /// reserve space to avoid multiple allocations

    /// iterate over the pattern, and convert to RPN using the shunting-yard algorithm
    ///
    for (char c : pattern.pattern) 
    {
        CharType type = GetType<OpEncoded>(c);
        switch(type)
        {
        case CharType::LITERAL:
{
            EXPECTS_THROW(expectOperand, "TODO: UNKERR?");
            ret.push_back(c);
            expectOperand = false;
            break;
        }
        case CharType::LPAREN:
        {
            opStack.push(OpEncoded::LPAREN);
            expectOperand = true;
            break;
        }
        case CharType::RPAREN:
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
        case CharType::BINARY_OP:
        case CharType::UNARY_OP:
        {
            EXPECTS_THROW(!expectOperand, "TODO: UNKERR?");

            /// pop off the stack until we find a lower precedence operator or a left paren
            ///
            while(!opStack.empty() && opStack.top() != OpEncoded::LPAREN &&
                  ((OP_PRIO.at(opStack.top()) > OP_PRIO.at((OpEncoded) c)) ||
                  (OP_PRIO.at(opStack.top()) == OP_PRIO.at((OpEncoded) c) && type == CharType::BINARY_OP))) 
            {
                ret.push_back((char)pop(opStack));
            }

            opStack.push((OpEncoded) c); // push the current operator onto the stack
            expectOperand = (type == CharType::BINARY_OP); // binary operator requries next symbol to be oprnd
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

    pattern.pattern = std::move(ret);
}

void PreProcessor::PrintRegex(std::ostream &os, const Pattern &pattern)
{
    if (!pattern.isRegex)
    {
        os << pattern.pattern;
    } 
    else 
    {
        for (char c : pattern.pattern)
        {
            os << Decode(c);
        }
    }
}

std::string PreProcessor::RegexStr(const Pattern &pattern)
{
    std::stringstream ss;
    PrintRegex(ss, pattern);
    return ss.str();
}