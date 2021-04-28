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

struct OutRegexData
{
	std::unordered_map<size_t, GraphElements::NodePtr> group_nodes; // Contains "GroupStart" or first internal group element.
};

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, OutRegexData& out_data, const RegexElementsChain& chain, const GraphElements::NodePtr& next);

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const AnySymbol&, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const SpecificSymbol& specific_symbol, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next, specific_symbol.code});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const OneOf& one_of, const GraphElements::NodePtr& next)
{
	GraphElements::OneOf out_node;
	out_node.next= next;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const Group& group, const GraphElements::NodePtr& next)
{
	const GroupStat& stat= group_stats.at(group.index);

	if(stat.indirect_call_count == 0)
	{
		if(stat.backreference_count == 0)
		{
			// No calls, no backreferences - generate only node for internal elements.
			const auto node= BuildRegexGraphChain(group_stats, out_data, group.elements, next);
			out_data.group_nodes[group.index]= node;
			return node;
		}
		else
		{
			// Have backreferences - generate start/end nodes.
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next, group.index});
			const auto group_contents= BuildRegexGraphChain(group_stats, out_data, group.elements, group_end);
			const auto group_start= std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
			out_data.group_nodes[group.index]= group_start;
			return group_start;
		}
	}
	else
	{
		// We do not need to save state here, save will be saved (if needed) in indirect call.

		if(stat.backreference_count == 0)
		{
			// No backreferences - generate only enter/leave nodes.
			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(group_stats, out_data, group.elements, subroutine_leave);
			out_data.group_nodes[group.index]= group_contents;
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_contents, group.index});
		}
		else
		{
			// Both backreferences and indirect calls needed - generated all nodes.

			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});

			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{subroutine_leave, group.index});
			const auto group_contents= BuildRegexGraphChain(group_stats, out_data, group.elements, group_end);
			const auto group_start= std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
			out_data.group_nodes[group.index]= group_start;
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_start, group.index});
		}
	}
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const BackReference& back_reference, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{next, back_reference.index});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const NonCapturingGroup& non_capturing_group, const GraphElements::NodePtr& next)
{
	return BuildRegexGraphChain(group_stats, out_data, non_capturing_group.elements, next);
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const AtomicGroup& atomic_group, const GraphElements::NodePtr& next)
{
	return
		std::make_shared<GraphElements::Node>(
			GraphElements::AtomicGroup{
				next,
				BuildRegexGraphChain(group_stats, out_data, atomic_group.elements, nullptr)});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const Alternatives& alternatives, const GraphElements::NodePtr& next)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphChain(group_stats, out_data, alternative, next));

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const Look& look, const GraphElements::NodePtr& next)
{
	GraphElements::Look out_node;
	out_node.next= next;
	out_node.look_graph= BuildRegexGraphChain(group_stats, out_data, look.elements, nullptr);
	out_node.forward= look.forward;
	out_node.positive= look.positive;

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const ConditionalElement& conditional_element, const GraphElements::NodePtr& next)
{
	GraphElements::ConditionalElement out_node;
	out_node.condition_node= BuildRegexGraphNodeImpl(group_stats, out_data, conditional_element.look, nullptr);
	out_node.next_true=  BuildRegexGraphChain(group_stats, out_data, conditional_element.alternatives.alternatives[0], next);
	out_node.next_false= BuildRegexGraphChain(group_stats, out_data, conditional_element.alternatives.alternatives[1], next);

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData& out_data, const RecursionGroup& recursion_group, const GraphElements::NodePtr& next)
{
	// We need to save and restore state in subroutine calls.
	// TODO - fill required for save/restore data.
	const auto state_restore= std::make_shared<GraphElements::Node>(GraphElements::StateRestore{next});
	const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{state_restore, nullptr /*set actual pointer later*/, recursion_group.index});
	return std::make_shared<GraphElements::Node>(GraphElements::StateSave{enter_node});
}

GraphElements::NodePtr BuildRegexGraphNode(const GroupStats& group_stats, OutRegexData& out_data, const RegexElementFull::ElementType& element, const GraphElements::NodePtr& next)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(group_stats, out_data, el, next); }, element);
}

using RegexChainIterator= RegexElementsChain::const_iterator;

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, const RegexChainIterator begin, const RegexChainIterator end, OutRegexData& out_data, const GraphElements::NodePtr& next)
{
	if(begin == end)
		return next;

	const RegexElementFull& element= *begin;

	const auto next_node= BuildRegexGraphChain(group_stats, std::next(begin), end, out_data, next);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
		return BuildRegexGraphNode(group_stats, out_data, element.el, next_node);
	else if(element.seq.mode == SequenceMode::Possessive)
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					BuildRegexGraphNode(group_stats, out_data, element.el, nullptr),
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

		const auto node= BuildRegexGraphNode(group_stats, out_data, element.el, loop_counter_block);

		std::get<GraphElements::LoopCounterBlock>(*loop_counter_block).next_iteration= node;

		return std::make_shared<GraphElements::Node>(GraphElements::LoopEnter{loop_counter_block, node, id});
	}
}

GraphElements::NodePtr BuildRegexGraphChain(const GroupStats& group_stats, OutRegexData& out_data, const RegexElementsChain& chain, const GraphElements::NodePtr& next)
{
	return BuildRegexGraphChain(group_stats, chain.begin(), chain.end(), out_data, next);
}

//
// SetupSubroutineCalls
//

void SetupSubroutineCalls(const GraphElements::NodePtr& node, const OutRegexData& regex_data);

template<typename T>
void SetupSubroutineCallsImpl(const T& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::Alternatives& node, const OutRegexData& regex_data)
{
	for(const auto& next : node.next)
		SetupSubroutineCalls(next, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::Look& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
	SetupSubroutineCalls(node.look_graph, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::ConditionalElement& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.condition_node, regex_data);
	SetupSubroutineCalls(node.next_true, regex_data);
	SetupSubroutineCalls(node.next_false, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::LoopEnter& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
	SetupSubroutineCalls(node.loop_iteration_node, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::LoopCounterBlock& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next_loop_end, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::PossessiveSequence& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
	SetupSubroutineCalls(node.sequence_element, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::AtomicGroup& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
	SetupSubroutineCalls(node.group_element, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::SubroutineEnter& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
	SetupSubroutineCalls(node.subroutine_node, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::SubroutineLeave& node, const OutRegexData& regex_data)
{
	(void)node;
	(void)regex_data;
}

void SetupSubroutineCalls(const GraphElements::NodePtr& node, const OutRegexData& regex_data)
{
	if(node == nullptr)
		return;

	if(const auto subroutine_enter= std::get_if<GraphElements::SubroutineEnter>(node.get()))
		if(subroutine_enter->subroutine_node == nullptr)
			subroutine_enter->subroutine_node= regex_data.group_nodes.at(subroutine_enter->index);

	return std::visit([&](const auto& el){ return SetupSubroutineCallsImpl(el, regex_data); }, *node);
}

} // namespace

GraphElements::NodePtr BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	GroupStats group_stats;
	CollectGroupInternalsForRegexChain(regex_chain, group_stats[0]);
	CollectGroupStatsForRegexChain(regex_chain, group_stats);
	SearchRecursiveGroupCalls(group_stats);

	GraphElements::NodePtr result_root;
	OutRegexData out_regex_data;
	if(group_stats.at(0).indirect_call_count == 0)
		result_root= BuildRegexGraphChain(group_stats, out_regex_data, regex_chain, nullptr);
	else
	{
		const auto end_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
		const auto node= BuildRegexGraphChain(group_stats, out_regex_data, regex_chain, end_node);
		out_regex_data.group_nodes[0]= node;
		result_root= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{nullptr, node, 0u});
	}

	SetupSubroutineCalls(result_root, out_regex_data);
	return result_root;
}

} // namespace RegPanzer
