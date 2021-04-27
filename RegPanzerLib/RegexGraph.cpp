#include "RegexGraph.hpp"

namespace RegPanzer
{

namespace
{

//
// CollectLoopIdsForRegexChain
//

GraphElements::LoopId GetLoopId(const RegexElementFull& element)
{
	return &element;
}

void CollectLoopIdsForRegexChain(const RegexElementsChain& regex_chain, GraphElements::LoopIdSet& loop_id_set);

void CollectLoopIdsForElementImpl(const AnySymbol&, GraphElements::LoopIdSet&){}
void CollectLoopIdsForElementImpl(const SpecificSymbol&, GraphElements::LoopIdSet&){}
void CollectLoopIdsForElementImpl(const OneOf&, GraphElements::LoopIdSet&){}

void CollectLoopIdsForElementImpl(const Group& group, GraphElements::LoopIdSet& loop_id_set)
{
	CollectLoopIdsForRegexChain(group.elements, loop_id_set);
}

void CollectLoopIdsForElementImpl(const BackReference&, GraphElements::LoopIdSet&){}

void CollectLoopIdsForElementImpl(const NonCapturingGroup& non_capturing_group, GraphElements::LoopIdSet& loop_id_set)
{
	CollectLoopIdsForRegexChain(non_capturing_group.elements, loop_id_set);
}

void CollectLoopIdsForElementImpl(const AtomicGroup& atomic_group, GraphElements::LoopIdSet& loop_id_set)
{
	CollectLoopIdsForRegexChain(atomic_group.elements, loop_id_set);
}

void CollectLoopIdsForElementImpl(const Look& look, GraphElements::LoopIdSet& loop_id_set)
{
	CollectLoopIdsForRegexChain(look.elements, loop_id_set);
}

void CollectLoopIdsForElementImpl(const Alternatives& alternatives, GraphElements::LoopIdSet& loop_id_set)
{
	for(const RegexElementsChain& alternaive : alternatives.alternatives)
		CollectLoopIdsForRegexChain(alternaive, loop_id_set);
}

void CollectLoopIdsForElementImpl(const ConditionalElement& conditional_element, GraphElements::LoopIdSet& loop_id_set)
{
	CollectLoopIdsForElementImpl(conditional_element.look, loop_id_set);
	CollectLoopIdsForElementImpl(conditional_element.alternatives, loop_id_set);
}

void CollectLoopIdsForElementImpl(const RecursionGroup&, GraphElements::LoopIdSet&){}

void CollectLoopIdsForElement(const RegexElementFull::ElementType& element, GraphElements::LoopIdSet& loop_id_set)
{
	std::visit([&](const auto& el){ return CollectLoopIdsForElementImpl(el, loop_id_set); }, element);
}

void CollectLoopIdsForRegexElement(const RegexElementFull& element_full, GraphElements::LoopIdSet& loop_id_set)
{
	loop_id_set.insert(GetLoopId(element_full));
	CollectLoopIdsForElement(element_full.el, loop_id_set);
}

void CollectLoopIdsForRegexChain(const RegexElementsChain& regex_chain, GraphElements::LoopIdSet& loop_id_set)
{
	for(const RegexElementFull& el : regex_chain)
		CollectLoopIdsForRegexElement(el, loop_id_set);
}

//
// BuildRegexGraphNode
//

using RegexChainIterator= RegexElementsChain::const_iterator;

GraphElements::NodePtr BuildRegexGraphNodeImpl(const RegexChainIterator begin, const RegexChainIterator end, const GraphElements::NodePtr& next);

GraphElements::NodePtr BuildRegexGraphNodeImpl(const AnySymbol&, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const SpecificSymbol& specific_symbol, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next, specific_symbol.code});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const OneOf& one_of, const GraphElements::NodePtr& next)
{
	GraphElements::OneOf out_node;
	out_node.next= next;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Group& group, const GraphElements::NodePtr& next)
{
	GraphElements::LoopIdSet loop_id_set;
	CollectLoopIdsForRegexChain(group.elements, loop_id_set);

	// TODO - set groups state restore mask.
	const size_t groups_mask= 0;

	const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{groups_mask, loop_id_set});

	const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{subroutine_leave, group.index});
	const auto group_contents= BuildRegexGraphNodeImpl(group.elements.begin(), group.elements.end(), group_end);
	const auto group_start= std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});

	return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_start, group.index, groups_mask, loop_id_set});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const BackReference& back_reference, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{next, back_reference.index});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const NonCapturingGroup& non_capturing_group, const GraphElements::NodePtr& next)
{
	return BuildRegexGraphNodeImpl(non_capturing_group.elements.begin(), non_capturing_group.elements.end(), next);
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const AtomicGroup& atomic_group, const GraphElements::NodePtr& next)
{
	return
		std::make_shared<GraphElements::Node>(
			GraphElements::AtomicGroup{
				next,
				BuildRegexGraphNodeImpl(atomic_group.elements.begin(), atomic_group.elements.end(), nullptr)});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Alternatives& alternatives, const GraphElements::NodePtr& next)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphNodeImpl(alternative.begin(), alternative.end(), next));

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const Look& look, const GraphElements::NodePtr& next)
{
	GraphElements::Look out_node;
	out_node.next= next;
	out_node.look_graph= BuildRegexGraphNodeImpl(look.elements.begin(), look.elements.end(), nullptr);
	out_node.forward= look.forward;
	out_node.positive= look.positive;

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const ConditionalElement& conditional_element, const GraphElements::NodePtr& next)
{
	GraphElements::ConditionalElement out_node;
	out_node.condition_node= BuildRegexGraphNodeImpl(conditional_element.look, nullptr);
	out_node.next_true=  BuildRegexGraphNodeImpl(conditional_element.alternatives.alternatives[0].begin(), conditional_element.alternatives.alternatives[0].end(), next);
	out_node.next_false= BuildRegexGraphNodeImpl(conditional_element.alternatives.alternatives[1].begin(), conditional_element.alternatives.alternatives[1].end(), next);

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const RecursionGroup& recursion_group, const GraphElements::NodePtr& next)
{
	// TODO - set groups state save mask.
	// TODO - set set of loop counters to restore.

	return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, nullptr /*set actual pointer later*/, recursion_group.index, 0, {}});
}

GraphElements::NodePtr BuildRegexGraphNode(const RegexElementFull::ElementType& element, const GraphElements::NodePtr& next)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(el, next); }, element);
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const RegexChainIterator begin, const RegexChainIterator end, const GraphElements::NodePtr& next)
{
	if(begin == end)
		return next;

	const RegexElementFull& element= *begin;

	const auto next_node= BuildRegexGraphNodeImpl(std::next(begin), end, next);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
		return BuildRegexGraphNode(element.el, next_node);
	else if(element.seq.mode == SequenceMode::Possessive)
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					BuildRegexGraphNode(element.el, nullptr),
					element.seq.min_elements,
					element.seq.max_elements,
					});
	else
	{
		const GraphElements::LoopId id= GetLoopId(element);

		const auto loop_counter_block=
			std::make_shared<GraphElements::Node>(
				GraphElements::LoopCounterBlock{
					GraphElements::NodePtr(),
					next_node,
					id,
					element.seq.min_elements,
					element.seq.max_elements,
					element.seq.mode != SequenceMode::Lazy,
					});

		const auto node= BuildRegexGraphNode(element.el, loop_counter_block);

		std::get<GraphElements::LoopCounterBlock>(*loop_counter_block).next_iteration= node;

		return std::make_shared<GraphElements::Node>(GraphElements::LoopEnter{loop_counter_block, node, id});
	}
}

//
// GetGroupNodeByIndex
//

GraphElements::NodePtr GetGroupNodeByIndex(const GraphElements::NodePtr& node, const size_t index);

template<typename T>
GraphElements::NodePtr GetGroupNodeByIndexImpl(const T& node, const size_t index)
{
	return GetGroupNodeByIndex(node.next, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::Alternatives& node, const size_t index)
{
	for(const auto& next : node.next)
		if(const auto res= GetGroupNodeByIndex(next, index))
			return res;

	return nullptr;
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::Look& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next, index))
		return res;
	return GetGroupNodeByIndex(node.look_graph, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::ConditionalElement& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next_true, index))
		return res;
	if(const auto res= GetGroupNodeByIndex(node.next_false, index))
		return res;
	return GetGroupNodeByIndex(node.condition_node, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::LoopEnter& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next, index))
		return res;
	return GetGroupNodeByIndex(node.loop_iteration_node, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::LoopCounterBlock& node, const size_t index)
{
	return GetGroupNodeByIndex(node.next_loop_end, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::PossessiveSequence& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next, index))
		return res;
	return GetGroupNodeByIndex(node.sequence_element, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::AtomicGroup& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next, index))
		return res;
	return GetGroupNodeByIndex(node.group_element, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::SubroutineEnter& node, const size_t index)
{
	if(const auto res= GetGroupNodeByIndex(node.next, index))
		return res;
	return GetGroupNodeByIndex(node.subroutine_node, index);
}

GraphElements::NodePtr GetGroupNodeByIndexImpl(const GraphElements::SubroutineLeave& node, const size_t index)
{
	(void)node;
	(void)index;
	return nullptr;
}

GraphElements::NodePtr GetGroupNodeByIndex(const GraphElements::NodePtr& node, const size_t index)
{
	if(node == nullptr)
		return nullptr;

	if(const auto group_start= std::get_if<GraphElements::GroupStart>(node.get()))
		if(group_start->index == index)
			return node;

	return std::visit([&](const auto& el){ return GetGroupNodeByIndexImpl(el, index); }, *node);
}

//
// SetupSubroutineCalls
//

void SetupSubroutineCalls(const GraphElements::NodePtr& node, const GraphElements::NodePtr& root);

template<typename T>
void SetupSubroutineCallsImpl(const T& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
}

void SetupSubroutineCallsImpl(const GraphElements::Alternatives& node, const GraphElements::NodePtr& root)
{
	for(const auto& next : node.next)
		SetupSubroutineCalls(next, root);
}

void SetupSubroutineCallsImpl(const GraphElements::Look& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
	SetupSubroutineCalls(node.look_graph, root);
}

void SetupSubroutineCallsImpl(const GraphElements::ConditionalElement& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.condition_node, root);
	SetupSubroutineCalls(node.next_true, root);
	SetupSubroutineCalls(node.next_false, root);
}

void SetupSubroutineCallsImpl(const GraphElements::LoopEnter& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
	SetupSubroutineCalls(node.loop_iteration_node, root);
}

void SetupSubroutineCallsImpl(const GraphElements::LoopCounterBlock& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next_loop_end, root);
}

void SetupSubroutineCallsImpl(const GraphElements::PossessiveSequence& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
	SetupSubroutineCalls(node.sequence_element, root);
}

void SetupSubroutineCallsImpl(const GraphElements::AtomicGroup& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
	SetupSubroutineCalls(node.group_element, root);
}

void SetupSubroutineCallsImpl(const GraphElements::SubroutineEnter& node, const GraphElements::NodePtr& root)
{
	SetupSubroutineCalls(node.next, root);
	SetupSubroutineCalls(node.subroutine_node, root);
}

void SetupSubroutineCallsImpl(const GraphElements::SubroutineLeave& node, const GraphElements::NodePtr& root)
{
	(void)node;
	(void)root;
}

void SetupSubroutineCalls(const GraphElements::NodePtr& node, const GraphElements::NodePtr& root)
{
	if(node == nullptr)
		return;

	if(const auto subroutine_enter= std::get_if<GraphElements::SubroutineEnter>(node.get()))
	{
		if(subroutine_enter->subroutine_node == nullptr)
			subroutine_enter->subroutine_node= GetGroupNodeByIndex(root, subroutine_enter->index);
	}

	return std::visit([&](const auto& el){ return SetupSubroutineCallsImpl(el, root); }, *node);
}

} // namespace

GraphElements::NodePtr BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	const auto end_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{~size_t(0), {}});
	const auto node= BuildRegexGraphNodeImpl(regex_chain.begin(), regex_chain.end(), end_node);
	const auto start= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{nullptr, node, 0u, 0u, {}});

	SetupSubroutineCalls(start, start);
	return start;
}

} // namespace RegPanzer
