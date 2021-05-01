#pragma once
#include "RegexElements.hpp"
#include <memory>
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
struct OneOf;
struct Alternatives;
struct GroupStart;
struct GroupEnd;
struct BackReference;
struct Look;
struct ConditionalElement;
struct LoopEnter;
struct LoopCounterBlock;
struct CounterlessLoopNode;
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
	OneOf,
	Alternatives,
	GroupStart,
	GroupEnd,
	BackReference,
	Look,
	ConditionalElement,
	LoopEnter,
	LoopCounterBlock,
	CounterlessLoopNode,
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

struct Look
{
	NodePtr next;
	NodePtr look_graph;
	bool forward= true;
	bool positive= true;
};

struct ConditionalElement
{
	NodePtr condition_node;
	NodePtr next_true;
	NodePtr next_false;
};

using LoopId= const void*;
using LoopIdSet= std::unordered_set<LoopId>;

struct LoopEnter
{
	NodePtr next; // To loop counter block.
	LoopId id= nullptr;
};

struct LoopCounterBlock
{
	NodePtr next_iteration;
	NodePtr next_loop_end;
	LoopId id= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	bool greedy= true;
};

struct CounterlessLoopNode
{
	NodePtr next_iteration;
	NodePtr next_loop_end;
	bool greedy= true;
};

// Used for creation of back references (loops, recursive calls, etc.)
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
	LoopIdSet loop_counters_to_save;
	std::unordered_set<size_t> groups_to_save;
};

struct StateRestore
{
	NodePtr next;
	LoopIdSet loop_counters_to_restore;
	std::unordered_set<size_t> groups_to_restore;
};

} // GraphElements

GraphElements::NodePtr BuildRegexGraph(const RegexElementsChain& regex_chain);

} // namespace RegPanzer
