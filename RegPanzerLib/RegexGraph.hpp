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
struct NextWeakNode;
struct PossessiveSequence;
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
	NextWeakNode,
	PossessiveSequence,
	AtomicGroup,
	SubroutineEnter,
	SubroutineLeave,
	StateSave,
	StateRestore>;

using NodePtr= std::shared_ptr<Node>;

struct AnySymbol
{
	NodePtr next;
};

struct SpecificSymbol
{
	NodePtr next;
	CharType code= 0;
};

struct String
{
	NodePtr next;
	std::basic_string<CharType> str;
};

struct OneOf
{
	NodePtr next;

	std::vector<CharType> variants;
	std::vector< std::pair<CharType, CharType> > ranges;
	bool inverse_flag= false;
};

struct Alternatives
{
	std::vector<NodePtr> next;
};

struct GroupStart
{
	NodePtr next; // To contents of the group.
	size_t index= std::numeric_limits<size_t>::max();
};

struct GroupEnd
{
	NodePtr next;
	size_t index= std::numeric_limits<size_t>::max();
};

struct BackReference
{
	NodePtr next;
	size_t index= std::numeric_limits<size_t>::max();
};

struct LookAhead
{
	NodePtr next;
	NodePtr look_graph;
	bool positive= true;
};

struct LookBehind
{
	NodePtr next;
	NodePtr look_graph;
	bool positive= true;
	size_t size= 0; // Now we support look behind with fixed size only.
};

struct StringStartAssertion
{
	NodePtr next;
};

struct StringEndAssertion
{
	NodePtr next;
};

struct ConditionalElement
{
	NodePtr condition_node;
	NodePtr next_true;
	NodePtr next_false;
};

using SequenceId= const void*;
using SequenceIdSet= std::unordered_set<SequenceId>;

struct SequenceCounterReset
{
	NodePtr next; // To sequence counter node.
	SequenceId id= nullptr;
};

struct SequenceCounter
{
	NodePtr next_iteration;
	NodePtr next_sequence_end;
	SequenceId id= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	bool greedy= true;
};

// Used for creation of back references (sequences, recursive calls, etc.)
struct NextWeakNode
{
	NodePtr::weak_type next;
};

struct PossessiveSequence
{
	NodePtr next;
	NodePtr sequence_element;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
};

struct AtomicGroup
{
	NodePtr next;
	NodePtr group_element;
};

struct SubroutineEnter
{
	NodePtr next; // Next node after subroutine leave.
	NodePtr subroutine_node;
	size_t index= std::numeric_limits<size_t>::max(); // 0 - whole expression, 1 - first group, etc.
};

struct SubroutineLeave
{
};

struct StateSave
{
	NodePtr next;
	SequenceIdSet sequence_counters_to_save;
	std::unordered_set<size_t> groups_to_save;
};

struct StateRestore
{
	NodePtr next;
	SequenceIdSet sequence_counters_to_restore;
	std::unordered_set<size_t> groups_to_restore;
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
	GraphElements::NodePtr root;
};

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain, const Options& options);

} // namespace RegPanzer
