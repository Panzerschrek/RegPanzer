#include "RegexGraph.hpp"
#include <unordered_map>

namespace RegPanzer
{

namespace
{

//
// CollectGroupInternalsForRegexChain
//

GraphElements::LoopId GetLoopId(const RegexElementFull& element)
{
	return &element;
}

using GroupIdSet= std::unordered_set<size_t>;
using CallTargetSet= std::unordered_set<size_t>;

struct GroupStat
{
	bool recursive= false; // Both directly and indirectly.
	size_t backreference_count= 0;
	size_t indirect_call_count= 0; // (?1), (?R), etc.
	GraphElements::LoopIdSet internal_loops;
	GroupIdSet internal_groups; // All (include children of children and futrher).
	CallTargetSet internal_calls; // All (include children of children and futrher).
};

void CollectGroupInternalsForRegexChain(const RegexElementsChain& regex_chain, GroupStat&);

void CollectGroupInternalsForElementImpl(const AnySymbol&, GroupStat&){}
void CollectGroupInternalsForElementImpl(const SpecificSymbol&, GroupStat&){}
void CollectGroupInternalsForElementImpl(const OneOf&, GroupStat&){}

void CollectGroupInternalsForElementImpl(const Group& group, GroupStat& stat)
{
	stat.internal_groups.insert(group.index);
	CollectGroupInternalsForRegexChain(group.elements, stat);
}

void CollectGroupInternalsForElementImpl(const BackReference&, GroupStat&){}

void CollectGroupInternalsForElementImpl(const NonCapturingGroup& non_capturing_group, GroupStat& stat)
{
	CollectGroupInternalsForRegexChain(non_capturing_group.elements, stat);
}

void CollectGroupInternalsForElementImpl(const AtomicGroup& atomic_group, GroupStat& stat)
{
	CollectGroupInternalsForRegexChain(atomic_group.elements, stat);
}

void CollectGroupInternalsForElementImpl(const Look& look, GroupStat& stat)
{
	CollectGroupInternalsForRegexChain(look.elements, stat);
}

void CollectGroupInternalsForElementImpl(const Alternatives& alternatives, GroupStat& stat)
{
	for(const RegexElementsChain& alternaive : alternatives.alternatives)
		CollectGroupInternalsForRegexChain(alternaive, stat);
}

void CollectGroupInternalsForElementImpl(const ConditionalElement& conditional_element, GroupStat& stat)
{
	CollectGroupInternalsForElementImpl(conditional_element.look, stat);
	CollectGroupInternalsForElementImpl(conditional_element.alternatives, stat);
}

void CollectGroupInternalsForElementImpl(const RecursionGroup& recursion_group, GroupStat& stat)
{
	stat.internal_calls.insert(recursion_group.index);
}

void CollectGroupIdsForElement(const RegexElementFull::ElementType& element, GroupStat& stat)
{
	std::visit([&](const auto& el){ return CollectGroupInternalsForElementImpl(el, stat); }, element);
}

void CollectGroupInternalsForRegexElement(const RegexElementFull& element_full, GroupStat& stat)
{
	stat.internal_loops.insert(GetLoopId(element_full));
	CollectGroupIdsForElement(element_full.el, stat);
}

void CollectGroupInternalsForRegexChain(const RegexElementsChain& regex_chain, GroupStat& stat)
{
	for(const RegexElementFull& el : regex_chain)
		CollectGroupInternalsForRegexElement(el, stat);
}

//
// CollectGroupStatsForRegexChain
//

using GroupStats= std::unordered_map<size_t, GroupStat>;

void CollectGroupStatsForRegexChain(const RegexElementsChain& regex_chain, GroupStats& group_stats);

void CollectGroupStatsForElementImpl(const AnySymbol&, GroupStats&){}
void CollectGroupStatsForElementImpl(const SpecificSymbol&, GroupStats&){}
void CollectGroupStatsForElementImpl(const OneOf&, GroupStats&){}

void CollectGroupStatsForElementImpl(const Group& group, GroupStats& group_stats)
{
	CollectGroupInternalsForRegexChain(group.elements, group_stats[group.index]);
	CollectGroupStatsForRegexChain(group.elements, group_stats);
}

void CollectGroupStatsForElementImpl(const BackReference& back_reference, GroupStats& group_stats)
{
	++(group_stats[back_reference.index].backreference_count);
}

void CollectGroupStatsForElementImpl(const NonCapturingGroup& non_capturing_group, GroupStats& group_stats)
{
	CollectGroupStatsForRegexChain(non_capturing_group.elements, group_stats);
}

void CollectGroupStatsForElementImpl(const AtomicGroup& atomic_group, GroupStats& group_stats)
{
	CollectGroupStatsForRegexChain(atomic_group.elements, group_stats);
}

void CollectGroupStatsForElementImpl(const Look& look, GroupStats& group_stats)
{
	CollectGroupStatsForRegexChain(look.elements, group_stats);
}

void CollectGroupStatsForElementImpl(const Alternatives& alternatives, GroupStats& group_stats)
{
	for(const RegexElementsChain& alternaive : alternatives.alternatives)
		CollectGroupStatsForRegexChain(alternaive, group_stats);
}

void CollectGroupStatsForElementImpl(const ConditionalElement& conditional_element, GroupStats& group_stats)
{
	CollectGroupStatsForElementImpl(conditional_element.look, group_stats);
	CollectGroupStatsForElementImpl(conditional_element.alternatives, group_stats);
}

void CollectGroupStatsForElementImpl(const RecursionGroup& recursion_group, GroupStats& group_stats)
{
	++(group_stats[recursion_group.index].indirect_call_count);
}

void CollectGroupStatsForElement(const RegexElementFull::ElementType& element, GroupStats& group_stats)
{
	std::visit([&](const auto& el){ return CollectGroupStatsForElementImpl(el, group_stats); }, element);
}

void CollectGroupStatsForRegexElement(const RegexElementFull& element_full, GroupStats& group_stats)
{
	CollectGroupStatsForElement(element_full.el, group_stats);
}

void CollectGroupStatsForRegexChain(const RegexElementsChain& regex_chain, GroupStats& group_stats)
{
	for(const RegexElementFull& el : regex_chain)
		CollectGroupStatsForRegexElement(el, group_stats);
}

//
// SearchRecursiveGroupCalls
//

using GrupsStack= std::vector<size_t>;

void SearchRecursiveGroupCalls_r(GroupStats& group_stats, const size_t current_group_id, GrupsStack& stack)
{
	GroupStat& group_stat= group_stats[current_group_id];

	for(const size_t prev_stack_id : stack)
	{
		if(prev_stack_id == current_group_id)
		{
			group_stats[current_group_id].recursive= true;
			return;
		}
	}

	stack.push_back(current_group_id);

	for(const size_t& internal_group_id : group_stat.internal_groups)
		SearchRecursiveGroupCalls_r(group_stats, internal_group_id, stack);
	for(const size_t& internal_group_id : group_stat.internal_calls)
		SearchRecursiveGroupCalls_r(group_stats, internal_group_id, stack);

	stack.pop_back();
}

void SearchRecursiveGroupCalls(GroupStats& group_stats)
{
	GrupsStack stack;
	SearchRecursiveGroupCalls_r(group_stats, 0, stack);
}

//
// BuildRegexGraphNode
//

using RegexChainIterator= RegexElementsChain::const_iterator;

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, const RegexElementsChain& chain, const GraphElements::NodePtr& next);

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const AnySymbol&, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const SpecificSymbol& specific_symbol, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next, specific_symbol.code});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const OneOf& one_of, const GraphElements::NodePtr& next)
{
	GraphElements::OneOf out_node;
	out_node.next= next;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const Group& group, const GraphElements::NodePtr& next)
{
	const GroupStat& stat= group_stats.at(group.index);

	if(stat.indirect_call_count == 0)
	{
		if(stat.backreference_count == 0)
		{
			// No calls, no backreferences - generate only node for internal elements.
			return BuildRegexGraphChain(group_stats, group.elements, next);
		}
		else
		{
			// Have backreferences - generate start/end nodes.
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next, group.index});
			const auto group_contents= BuildRegexGraphChain(group_stats, group.elements, group_end);
			return std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
		}
	}
	else
	{
		// We do not need to save state here, save will be saved (if needed) in indirect call.

		if(stat.backreference_count == 0)
		{
			// No backreferences - generate only enter/leave nodes.
			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(group_stats, group.elements, subroutine_leave);
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_contents, group.index});
		}
		else
		{
			// Both backreferences and indirect calls needed - generated all nodes.

			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});

			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{subroutine_leave, group.index});
			const auto group_contents= BuildRegexGraphChain(group_stats, group.elements, group_end);
			const auto group_start= std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});

			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_start, group.index});
		}
	}
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const BackReference& back_reference, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{next, back_reference.index});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const NonCapturingGroup& non_capturing_group, const GraphElements::NodePtr& next)
{
	return BuildRegexGraphChain(group_stats, non_capturing_group.elements, next);
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const AtomicGroup& atomic_group, const GraphElements::NodePtr& next)
{
	return
		std::make_shared<GraphElements::Node>(
			GraphElements::AtomicGroup{
				next,
				BuildRegexGraphChain(group_stats, atomic_group.elements, nullptr)});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const Alternatives& alternatives, const GraphElements::NodePtr& next)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphChain(group_stats, alternative, next));

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const Look& look, const GraphElements::NodePtr& next)
{
	GraphElements::Look out_node;
	out_node.next= next;
	out_node.look_graph= BuildRegexGraphChain(group_stats, look.elements, nullptr);
	out_node.forward= look.forward;
	out_node.positive= look.positive;

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const ConditionalElement& conditional_element, const GraphElements::NodePtr& next)
{
	GraphElements::ConditionalElement out_node;
	out_node.condition_node= BuildRegexGraphNodeImpl(group_stats, conditional_element.look, nullptr);
	out_node.next_true=  BuildRegexGraphChain(group_stats, conditional_element.alternatives.alternatives[0], next);
	out_node.next_false= BuildRegexGraphChain(group_stats, conditional_element.alternatives.alternatives[1], next);

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, const RecursionGroup& recursion_group, const GraphElements::NodePtr& next)
{
	// We need to save and restore state in subroutine calls.
	// TODO - fill required for save/restore data.
	const auto state_restore= std::make_shared<GraphElements::Node>(GraphElements::StateRestore{next});
	const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{state_restore, nullptr /*set actual pointer later*/, recursion_group.index});
	return std::make_shared<GraphElements::Node>(GraphElements::StateSave{enter_node});
}

GraphElements::NodePtr BuildRegexGraphNode(const GroupStats& group_stats, const RegexElementFull::ElementType& element, const GraphElements::NodePtr& next)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(group_stats, el, next); }, element);
}

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, const RegexChainIterator begin, const RegexChainIterator end, const GraphElements::NodePtr& next)
{
	if(begin == end)
		return next;

	const RegexElementFull& element= *begin;

	const auto next_node= BuildRegexGraphChain(group_stats, std::next(begin), end, next);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
		return BuildRegexGraphNode(group_stats, element.el, next_node);
	else if(element.seq.mode == SequenceMode::Possessive)
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					BuildRegexGraphNode(group_stats, element.el, nullptr),
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

		const auto node= BuildRegexGraphNode(group_stats, element.el, loop_counter_block);

		std::get<GraphElements::LoopCounterBlock>(*loop_counter_block).next_iteration= node;

		return std::make_shared<GraphElements::Node>(GraphElements::LoopEnter{loop_counter_block, node, id});
	}
}

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, const RegexElementsChain& chain, const GraphElements::NodePtr& next)
{
	return BuildRegexGraphChain(group_stats, chain.begin(), chain.end(), next);
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

	// TODO - what if we do not add "GroupStart" in group node?
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
	GroupStats group_stats;
	CollectGroupInternalsForRegexChain(regex_chain, group_stats[0]);
	CollectGroupStatsForRegexChain(regex_chain, group_stats);
	SearchRecursiveGroupCalls(group_stats);

	GraphElements::NodePtr result_root;
	if(group_stats.at(0).indirect_call_count == 0)
		result_root= BuildRegexGraphChain(group_stats, regex_chain, nullptr);
	else
	{
		const auto end_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
		const auto node= BuildRegexGraphChain(group_stats, regex_chain, end_node);
		result_root= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{nullptr, node, 0u});
	}

	SetupSubroutineCalls(result_root, result_root);
	return result_root;
}

} // namespace RegPanzer
