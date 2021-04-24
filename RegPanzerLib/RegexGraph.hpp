#pragma once
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

using Node= std::variant<
	AnySymbol,
	SpecificSymbol,
	OneOf>;

using NodePtr= std::shared_ptr<const Node>;

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

struct LoopEnter
{
	NodePtr next; // To loop counter block.
	size_t index= std::numeric_limits<size_t>::max();
};

struct LoopCounterBlock
{
	NodePtr::weak_type next_iteration;
	NodePtr next_loop_end;
	size_t index= std::numeric_limits<size_t>::max();
	size_t min_elements= 0u;
	size_t max_elements= 0u;
};

} // GraphElements

} // namespace RegPanzer
