#include "RegexGraph.hpp"
#include <cassert>

namespace RegPanzer
{

namespace
{

using RegexChainIterator= RegexElementsChain::const_iterator;


void SetMostRightNext(GraphElements::Node& node, const GraphElements::NodePtr& next);

template<typename T>
void SetMostRightNextImpl(T& node, const GraphElements::NodePtr& next)
{
	if(node.next == nullptr)
		node.next= next;
	else
		SetMostRightNext(*node.next, next);
}

void SetMostRightNextImpl(GraphElements::Alternatives& node, const GraphElements::NodePtr& next)
{
	for(GraphElements::NodePtr& alternative : node.next)
	{
		if(alternative == nullptr)
			alternative= next;
		else
			SetMostRightNext(*alternative, next);
	}
}

void SetMostRightNextImpl(GraphElements::LoopCounterBlock& node, const GraphElements::NodePtr& next)
{
	if(node.next_loop_end == nullptr)
		node.next_loop_end= next;
	else
		SetMostRightNext(*node.next_loop_end, next);
}

void SetMostRightNext(GraphElements::Node& node, const GraphElements::NodePtr& next)
{
	if(&node == next.get())
		return; // Hack for alternatives.

	std::visit([&](auto& el){ return SetMostRightNextImpl(el, next); }, node);
}

GraphElements::NodePtr BuildRegexGraphImpl(const RegexChainIterator begin, const RegexChainIterator end);

GraphElements::NodePtr BuildRegexGraphNodeImpl(const AnySymbol&)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const SpecificSymbol& specific_symbol)
{
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{nullptr, specific_symbol.code});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const OneOf& one_of)
{
	GraphElements::OneOf out_node;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Group& group)
{
	const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{nullptr, group.index});
	const auto group_contents= BuildRegexGraphImpl(group.elements.begin(), group.elements.end());
	SetMostRightNext(*group_contents, group_end);

	return std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const BackReference& back_reference)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{nullptr, back_reference.index});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const NonCapturingGroup& non_capturing_group)
{
	return BuildRegexGraphImpl(non_capturing_group.elements.begin(), non_capturing_group.elements.end());
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const AtomicGroup& atomic_group)
{
	// TODO
	(void)atomic_group;
	assert(false);
	return nullptr;
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Alternatives& alternatives)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphImpl(alternative.begin(), alternative.end()));

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Look& look)
{
	GraphElements::Look out_node;
	out_node.look_graph= BuildRegexGraphImpl(look.elements.begin(), look.elements.end());
	out_node.forward= look.forward;
	out_node.positive= look.positive;

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNode(const RegexElementFull::ElementType& element)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(el); }, element);
}

GraphElements::NodePtr BuildRegexGraphImpl(const RegexChainIterator begin, const RegexChainIterator end)
{
	if(begin == end)
		return nullptr;

	const RegexElementFull& element= *begin;

	const auto node= BuildRegexGraphNode(element.el);
	const auto next_node= BuildRegexGraphImpl(std::next(begin), end);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
	{
		SetMostRightNext(*node, next_node);
		return node;
	}
	else
	{
		const GraphElements::LoopId id= &element;

		const auto loop_counter_block=
			std::make_shared<GraphElements::Node>(
				GraphElements::LoopCounterBlock{
					node,
					next_node,
					id,
					element.seq.min_elements,
					element.seq.max_elements,
					element.seq.mode != SequenceMode::Lazy,
					});

		SetMostRightNext(*node, loop_counter_block);
		return std::make_shared<GraphElements::Node>(GraphElements::LoopEnter{loop_counter_block, id});
	}
}

} // namespace

GraphElements::NodePtr BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	return BuildRegexGraphImpl(regex_chain.begin(), regex_chain.end());
}

} // namespace RegPanzer
