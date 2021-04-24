#pragma once
#include "RegexElements.hpp"
#include <memory>
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
struct LoopEnter;
struct LoopCounterBlock;

using Node= std::variant<
	AnySymbol,
	SpecificSymbol,
	OneOf,
	Alternatives,
	GroupStart,
	GroupEnd,
	BackReference,
	LoopEnter,
	LoopCounterBlock>;

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

using LoopId= const void*;

struct LoopEnter
{
	NodePtr next; // To loop counter block.
	LoopId id= nullptr;
};

struct LoopCounterBlock
{
	NodePtr::weak_type next_iteration;
	NodePtr next_loop_end;
	LoopId id= nullptr;
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	bool greedy= true;
};

} // GraphElements

GraphElements::NodePtr BuildRegexGraph(const RegexElementsChain& regex_chain);

} // namespace RegPanzer
