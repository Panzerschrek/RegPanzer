#pragma once
#include "Options.hpp"
#include "RegexElements.hpp"
#include <memory>
#include <map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace RegPanzer
{

namespace GraphElements
{

using CharType= char32_t;

struct AnySymbol;
struct SpecificSymbol;
struct String;
struct OneOf;
struct Alternatives;
struct AlternativesPossessive;
struct GroupStart;
struct GroupEnd;
struct BackReference;
struct LookAhead;
struct LookBehind;
struct StringStartAssertion;
struct StringEndAssertion;
struct ConditionalElement;
struct SequenceCounterReset;
struct SequenceCounter;
struct PossessiveSequence;
struct SingleRollbackPointSequence;
struct FixedLengthElementSequence;
struct AtomicGroup;
struct SubroutineEnter;
struct SubroutineLeave;
struct StateSave;
struct StateRestore;

using Node= std::variant<
	AnySymbol,
	SpecificSymbol,
	String,
	OneOf,
	Alternatives,
	AlternativesPossessive,
	GroupStart,
	GroupEnd,
	BackReference,
	LookAhead,
	LookBehind,
	StringStartAssertion,
	StringEndAssertion,
	ConditionalElement,
	SequenceCounterReset,
	SequenceCounter,
	PossessiveSequence,
	SingleRollbackPointSequence,
	FixedLengthElementSequence,
	AtomicGroup,
	SubroutineEnter,
	SubroutineLeave,
	StateSave,
	StateRestore>;

// Just observer ptr.
using NodePtr= Node*;

struct AnySymbol
{
	NodePtr next= nullptr;
};

// TODO - maybe replace it with "string" node?
struct SpecificSymbol
{
	NodePtr next= nullptr;
	CharType code= 0;
};

struct String
{
	NodePtr next= nullptr;
	std::string str; // UTF-8
};

struct OneOf
{
	NodePtr next= nullptr;

	std::vector<CharType> variants;
	std::vector< std::pair<CharType, CharType> > ranges;
	bool inverse_flag= false;
};

struct Alternatives
{
	std::vector<NodePtr> next;
};

struct AlternativesPossessive
{
	// Save state, evaluate path0_element,
	// if it is true - evaluate path0_next without state restoring,
	// else - restore state and evaluate path1_next.
	NodePtr path0_element= nullptr;
	NodePtr path0_next= nullptr;
	NodePtr path1_next= nullptr;
};

struct GroupStart
{
	NodePtr next= nullptr; // To contents of the group.
	size_t index= std::numeric_limits<size_t>::max();
};

struct GroupEnd
{
	NodePtr next= nullptr;
	size_t index= std::numeric_limits<size_t>::max();
};

struct BackReference
{
	NodePtr next= nullptr;
	size_t index= std::numeric_limits<size_t>::max();
};

struct LookAhead
{
	NodePtr next= nullptr;
	NodePtr look_graph= nullptr;
	bool positive= true;
};

struct LookBehind
{
	NodePtr next= nullptr;
	NodePtr look_graph= nullptr;
	bool positive= true;
	size_t size= 0; // Size in UTF-8 bytes. Now we support look behind with fixed size only.
};

struct StringStartAssertion
{
	NodePtr next= nullptr;
};

struct StringEndAssertion
{
	NodePtr next= nullptr;
};

struct ConditionalElement
{
	NodePtr condition_node= nullptr;
	NodePtr next_true= nullptr;
	NodePtr next_false= nullptr;
};

using SequenceId= const void*;
using SequenceIdSet= std::unordered_set<SequenceId>;

struct SequenceCounterReset
{
	NodePtr next= nullptr; // To sequence counter node.
	SequenceId id= nullptr;
};

struct SequenceCounter
{
	NodePtr next_iteration= nullptr;
	NodePtr next_sequence_end= nullptr;
	SequenceId id= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	bool greedy= true;
};

struct PossessiveSequence
{
	NodePtr next= nullptr;
	NodePtr sequence_element= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
};

struct SingleRollbackPointSequence
{
	NodePtr next= nullptr;
	NodePtr sequence_element= nullptr;
};

struct FixedLengthElementSequence
{
	NodePtr next= nullptr;
	NodePtr sequence_element= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	size_t element_length= 0u; // In UTF-8 bytes.
};

struct AtomicGroup
{
	NodePtr next= nullptr;
	NodePtr group_element= nullptr;
};

struct SubroutineEnter
{
	NodePtr next= nullptr; // Next node after subroutine leave.
	NodePtr subroutine_node= nullptr;
	size_t index= std::numeric_limits<size_t>::max(); // 0 - whole expression, 1 - first group, etc.
};

struct SubroutineLeave
{
};

struct StateSave
{
	NodePtr next= nullptr;
	SequenceIdSet sequence_counters_to_save;
	std::unordered_set<size_t> groups_to_save;
};

struct StateRestore
{
	NodePtr next= nullptr;
	SequenceIdSet sequence_counters_to_restore;
	std::unordered_set<size_t> groups_to_restore;
};

// Store all nodes inside this class.
// Use only observer pointers inside nodes itself.
// This is needed to prevent strong shared_ptr loops.
// All nodes are destroyed when this storage is destroyed.
// Noded deletion and any garbage collection is not supported,
// so, optimized-out nodes will be destroyed only during destruction of this class.
class NodesStorage
{
public:
	NodesStorage()= default;

	NodesStorage(const NodesStorage&)= delete;
	NodesStorage(NodesStorage&&)= default;

	NodesStorage& operator=(const NodesStorage&)= delete;
	NodesStorage& operator=(NodesStorage&&)= default;

public:
	NodePtr Allocate(Node node)
	{
		nodes_.push_back(std::make_unique<Node>(std::move(node)));
		return nodes_.back().get();
	}

private:
	std::vector<std::unique_ptr<Node>> nodes_;
};

} // GraphElements

using GroupIdSet= std::unordered_set<size_t>;
using CallTargetSet= std::unordered_set<size_t>;

struct GroupStat
{
	bool recursive= false; // Both directly and indirectly.
	size_t backreference_count= 0;
	size_t indirect_call_count= 0; // (?1), (?R), etc.
	GraphElements::SequenceIdSet internal_sequences; // Only sequences where "SequenceCounter" collected here.
	GroupIdSet internal_groups; // All (include children of children and futrher).
	CallTargetSet internal_calls; // All calls (include calls in children groups and children of children and futrher).
};

using GroupStats= std::map<size_t, GroupStat>;

struct RegexGraphBuildResult
{
	Options options;
	GroupStats group_stats;
	GraphElements::SequenceIdSet used_sequence_counters; // Set of sequence counters, actually used in this graph.
	GraphElements::NodePtr root= nullptr;
	GraphElements::NodesStorage nodes_storage;
};

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain, const Options& options);

} // namespace RegPanzer
