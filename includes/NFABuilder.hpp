/// @file NFABuilder.hpp
/// @brief NFA builder class

#include <cstddef>
#include <vector>

#include "NFA.hpp"

struct Pattern;

class NFABuilder
{
public:
    /// -----------------------------------------------------------------------
    /// Explicitly delete constructors, destructor and operator=
    /// -----------------------------------------------------------------------
    NFABuilder() = delete;
    ~NFABuilder() = delete;
    NFABuilder(const NFABuilder&) = delete;
    NFABuilder(const NFABuilder&&) = delete;
    NFABuilder& operator=(const NFABuilder&) = delete;
    NFABuilder& operator=(const NFABuilder&&) = delete;

    /// -----------------------------------------------------------------------
    /// Public api methods and constants
    /// -----------------------------------------------------------------------

    /// @brief The maximum allowed states allowed for a generated NFA
    static constexpr size_t MAX_STATE_COUNT = 500'000;

    /// @brief method to construct an nfa from pre-processed patterns
    /// @note If the patterns are not passed through the preprocessor, they 
    ///       will be treated as strings
    /// @param preProcessedPatterns patterns that have been pre-processed
    /// @return the NFA constructed from the patterns
    static NFA Build(const std::vector<Pattern>& preProcessedPatterns);

private:
    /// @brief struct representing a fragment. a fragment has an index, but
    ///        has transitions that are waiting to be filled
    struct Fragment
    {
        /// @brief struct representing a fragment's hole (to be patched)
        struct Hole
        {
            size_t holeIndex; ///< index of the state needing a transition
            char tVal; ///< known transition value from holeIndex 
        };
        size_t startIndex; ///< index of the starting state in this fragment 
        std::vector<Hole> holes; ///< transitions to be patched
    };

    /// @brief method to create a new state with no rule tag and add it to
    ///        the state vector
    /// @param nfaStates the state vector to add the new state to
    /// @return the index of the new state added (previous size of the array)
    static size_t NewState(std::vector<NFA::State>& nfaStates);
    
    /// @brief method to create a new state with a rule tag and add it to
    ///        the state vector
    /// @param ruleNo the rule number to tag the state with
    /// @param nfaStates the state vector to add the new state to
    /// @return the index of the new state added (previous size of the array)
    static size_t NewState(size_t ruleNo, std::vector<NFA::State>& nfaStates);

    /// @brief method to create a literal fragment
    /// @param a the transition (literal) value
    /// @param nfaStates the state vector of the current nfa
    /// @return 
    static Fragment MakeLiteral(char a, std::vector<NFA::State>& nfaStates);

    /// @brief method to apply a concatination operator to two fragments
    /// @param left the left fragment 
    /// @param right the right fragment
    /// @return the concatination of left->right
    static Fragment ApplyCat(const Fragment& left, const Fragment& right,
        std::vector<NFA::State>& states);

    static Fragment ApplyUnion(const Fragment& left, const Fragment& right,
        std::vector<NFA::State>& states);
    
    static void PatchHoles(const std::vector<Fragment::Hole>& holes,
        size_t patchIndex, std::vector<NFA::State>& states );
};