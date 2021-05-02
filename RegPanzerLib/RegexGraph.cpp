#include "RegexGraph.hpp"

namespace RegPanzer
{

namespace
{

//
// CollectGroupInternalsForRegexChain
//

GraphElements::SequenceId GetSequenceId(const RegexElementFull& element)
{
	return &element;
}

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

void CollectGroupInternalsForElementImpl(const SubroutineCall& subroutine_call, GroupStat& stat)
{
	stat.internal_calls.insert(subroutine_call.index);
}

void CollectGroupIdsForElement(const RegexElementFull::ElementType& element, GroupStat& stat)
{
	std::visit([&](const auto& el){ return CollectGroupInternalsForElementImpl(el, stat); }, element);
}

void CollectGroupInternalsForRegexElement(const RegexElementFull& element_full, GroupStat& stat)
{
	// If this changed, "BuildRegexGraphChain" function must be changed too!
	if(!(
		element_full.seq.mode == SequenceMode::Possessive ||
		(element_full.seq.min_elements == 1 && element_full.seq.max_elements == 1) ||
		(element_full.seq.min_elements == 0 && element_full.seq.max_elements == 1) ||
		(element_full.seq.min_elements == 0 && element_full.seq.max_elements == Sequence::c_max) ||
		(element_full.seq.min_elements == 1 && element_full.seq.max_elements == Sequence::c_max)))
	{
		stat.internal_sequences.insert(GetSequenceId(element_full));
	}

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

void CollectGroupStatsForElementImpl(const SubroutineCall& subroutine_call, GroupStats& group_stats)
{
	++(group_stats[subroutine_call.index].indirect_call_count);
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

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats&, OutRegexData&, const AnySymbol&, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats&, OutRegexData&, const SpecificSymbol& specific_symbol, const GraphElements::NodePtr& next)
{
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next, specific_symbol.code});
}

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats&, OutRegexData&, const OneOf& one_of, const GraphElements::NodePtr& next)
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

	// TODO -if building regexpr for groups extraction, always emit GroupStart/GroupEnd.

	if(stat.indirect_call_count == 0)
	{
		if(stat.backreference_count == 0)
		{
			// No calls, no backreferences - generate only node for internal elements.
			return BuildRegexGraphChain(group_stats, out_data, group.elements, next);
		}
		else
		{
			// Have backreferences - generate start/end nodes.
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next, group.index});
			const auto group_contents= BuildRegexGraphChain(group_stats, out_data, group.elements, group_end);
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

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats&, OutRegexData&, const BackReference& back_reference, const GraphElements::NodePtr& next)
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

GraphElements::NodePtr BuildRegexGraphNodeImpl(const GroupStats& group_stats, OutRegexData&, const SubroutineCall& subroutine_call, const GraphElements::NodePtr& next)
{
	// We need to save and restore state in subroutine calls.

	const GroupStat& stat= group_stats.at(subroutine_call.index);

	// Save sequences only if recursive calls possible.
	GraphElements::SequenceIdSet sequences;
	if(stat.recursive)
		sequences= stat.internal_sequences;

	// Save group state itself and state of its subgroups, but only if they used in backreferences.
	// TODO - save groups also if we match subexpressions.
	GroupIdSet groups;

	for(const size_t& internal_group_id : stat.internal_groups)
		if(group_stats.at(internal_group_id).backreference_count > 0)
			groups.insert(internal_group_id);

	if(group_stats.at(subroutine_call.index).backreference_count > 0)
		groups.insert(subroutine_call.index);

	if(sequences.empty() && groups.empty())
		return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, nullptr /*Set actual pointer value later*/, subroutine_call.index});
	else
	{
		const auto state_restore= std::make_shared<GraphElements::Node>(GraphElements::StateRestore{next, sequences, groups});

		const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{state_restore, nullptr /*Set actual pointer value later*/, subroutine_call.index});
		return std::make_shared<GraphElements::Node>(GraphElements::StateSave{enter_node, sequences, groups});
	}
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

	// If this changed, "CollectGroupInternalsForRegexElement" function must be changed too!

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
	else if(element.seq.min_elements == 0 && element.seq.max_elements == 1)
	{
		// Implement optional element using alternatives node.
		GraphElements::Alternatives alternatives;

		const auto node= BuildRegexGraphNode(group_stats, out_data, element.el, next_node);
		if(element.seq.mode == SequenceMode::Greedy)
		{
			alternatives.next.push_back(node);
			alternatives.next.push_back(next_node);
		}
		else
		{
			alternatives.next.push_back(next_node);
			alternatives.next.push_back(node);
		}

		return std::make_shared<GraphElements::Node>(std::move(alternatives));
	}
	else if(element.seq.min_elements == 1 && element.seq.max_elements == Sequence::c_max)
	{
		// In case of one or more elemenst first enter sequence body node, then alternatives node.

		const auto alternatives_node= std::make_shared<GraphElements::Node>(GraphElements::Alternatives{{next_node}});
		const auto node= BuildRegexGraphNode(group_stats, out_data, element.el, alternatives_node);
		const auto node_weak= std::make_shared<GraphElements::Node>(GraphElements::NextWeakNode{node});

		auto& alternatives= std::get<GraphElements::Alternatives>(*alternatives_node);
		if(element.seq.mode == SequenceMode::Lazy)
			alternatives.next.push_back(node_weak);
		else
			alternatives.next.insert(alternatives.next.begin(), node_weak);

		return node;
	}
	else if(element.seq.min_elements == 0 && element.seq.max_elements == Sequence::c_max)
	{
		// In case of zero or more elements first enter alternatives node, than sequence body node.

		const auto alternatives_node= std::make_shared<GraphElements::Node>(GraphElements::Alternatives{{next_node}});
		const auto alternatives_node_node_weak= std::make_shared<GraphElements::Node>(GraphElements::NextWeakNode{alternatives_node});
		const auto node= BuildRegexGraphNode(group_stats, out_data, element.el, alternatives_node_node_weak);

		auto& alternatives= std::get<GraphElements::Alternatives>(*alternatives_node);
		if(element.seq.mode == SequenceMode::Lazy)
			alternatives.next.push_back(node);
		else
			alternatives.next.insert(alternatives.next.begin(), node);

		return alternatives_node;
	}
	else
	{
		const GraphElements::SequenceId id= GetSequenceId(element);

		const auto sequence_counter_block=
			std::make_shared<GraphElements::Node>(
				GraphElements::SequenceCounter{
					GraphElements::NodePtr(),
					next_node,
					id,
					element.seq.min_elements,
					element.seq.max_elements,
					element.seq.mode != SequenceMode::Lazy,
					});

		const auto sequence_counter_block_weak= std::make_shared<GraphElements::Node>(GraphElements::NextWeakNode{sequence_counter_block});
		const auto node= BuildRegexGraphNode(group_stats, out_data, element.el, sequence_counter_block_weak);

		std::get<GraphElements::SequenceCounter>(*sequence_counter_block).next_iteration= node;

		return std::make_shared<GraphElements::Node>(GraphElements::SequenceCounterReset{sequence_counter_block, id});
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

void SetupSubroutineCallsImpl(const GraphElements::SequenceCounterReset& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::SequenceCounter& node, const OutRegexData& regex_data)
{
	SetupSubroutineCalls(node.next_iteration, regex_data);
	SetupSubroutineCalls(node.next_sequence_end, regex_data);
}

void SetupSubroutineCallsImpl(const GraphElements::NextWeakNode&, const OutRegexData&){}

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
	SetupSubroutineCalls(node.subroutine_node, regex_data);
	SetupSubroutineCalls(node.next, regex_data);
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
			subroutine_enter->subroutine_node=
				std::make_shared<GraphElements::Node>(
					GraphElements::NextWeakNode{regex_data.group_nodes.at(subroutine_enter->index)});

	std::visit([&](const auto& el){ return SetupSubroutineCallsImpl(el, regex_data); }, *node);
}

} // namespace

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	RegexGraphBuildResult res;

	CollectGroupInternalsForRegexChain(regex_chain, res.group_stats[0]);
	CollectGroupStatsForRegexChain(regex_chain, res.group_stats);
	SearchRecursiveGroupCalls(res.group_stats);

	OutRegexData out_regex_data;
	if(res.group_stats.at(0).indirect_call_count == 0)
		res.root= BuildRegexGraphChain(res.group_stats, out_regex_data, regex_chain, nullptr);
	else
	{
		const auto end_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
		const auto node= BuildRegexGraphChain(res.group_stats, out_regex_data, regex_chain, end_node);
		out_regex_data.group_nodes[0]= node;
		res.root= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{nullptr, node, 0u});
	}

	SetupSubroutineCalls(res.root, out_regex_data);
	return res;
}

} // namespace RegPanzer
