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
// GetRegexChainSize
//

using MinMaxSize= std::pair<size_t, size_t>;

MinMaxSize GetRegexChainSize(const RegexElementsChain& regex_chain);

MinMaxSize GetRegexElementSize_impl(const AnySymbol&)
{
	return MinMaxSize{1, 1};
}

MinMaxSize GetRegexElementSize_impl(const SpecificSymbol&)
{
	return MinMaxSize{1, 1};
}

MinMaxSize GetRegexElementSize_impl(const OneOf&)
{
	return MinMaxSize{1, 1};
}

MinMaxSize GetRegexElementSize_impl(const Group& group)
{
	return GetRegexChainSize(group.elements);
}

MinMaxSize GetRegexElementSize_impl(const BackReference&)
{
	return MinMaxSize{0, Sequence::c_max};
}

MinMaxSize GetRegexElementSize_impl(const NonCapturingGroup& group)
{
	return GetRegexChainSize(group.elements);
}

MinMaxSize GetRegexElementSize_impl(const AtomicGroup& group)
{
	return GetRegexChainSize(group.elements);
}

MinMaxSize GetRegexElementSize_impl(const Alternatives& alternatives)
{
	MinMaxSize s{Sequence::c_max, 0};
	for(const RegexElementsChain& alternative : alternatives.alternatives)
	{
		const auto el_s= GetRegexChainSize(alternative);
		s.first= std::min(s.first, el_s.first);
		s.second= std::max(s.second, el_s.second);
	}
	return s;
}

MinMaxSize GetRegexElementSize_impl(const Look&)
{
	return MinMaxSize{0, 0};
}

MinMaxSize GetRegexElementSize_impl(const ConditionalElement& conditional_element)
{
	return GetRegexElementSize_impl(conditional_element.alternatives);
}

MinMaxSize GetRegexElementSize_impl(const SubroutineCall&)
{
	return MinMaxSize{0, Sequence::c_max};
}

MinMaxSize GetRegexElementSize(const RegexElementFull& element)
{
	MinMaxSize el_size= std::visit([&](const auto& el){ return GetRegexElementSize_impl(el); }, element.el);

	el_size.first*= element.seq.min_elements;
	if(element.seq.max_elements == Sequence::c_max)
		el_size.second= Sequence::c_max;
	else
		el_size.second*= element.seq.max_elements;

	return el_size;
}

MinMaxSize GetRegexChainSize(const RegexElementsChain& regex_chain)
{
	MinMaxSize s{0, 0};
	for(const RegexElementFull& el : regex_chain)
	{
		const auto el_s= GetRegexElementSize(el);
		s.first+= el_s.first;
		s.second+= el_s.second;
	}

	return s;
}

//
// BuildRegexGraphNode
//

class RegexGraphBuilder
{
public:
	RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain);

private:
	using RegexChainIterator= RegexElementsChain::const_iterator;

	GraphElements::NodePtr BuildRegexGraphChain(const GraphElements::NodePtr& next,  const RegexElementsChain& chain);
	GraphElements::NodePtr BuildRegexGraphChain(const GraphElements::NodePtr& next,  RegexChainIterator begin, RegexChainIterator end);

	GraphElements::NodePtr BuildRegexGraphNode(const GraphElements::NodePtr& next,  const RegexElementFull::ElementType& element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const AnySymbol&);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const SpecificSymbol& specific_symbol);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const OneOf& one_of);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const Group& group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const BackReference& back_reference);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const NonCapturingGroup& non_capturing_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const AtomicGroup& atomic_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const Alternatives& alternatives);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const Look& look);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const ConditionalElement& conditional_element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next,  const SubroutineCall& subroutine_call);

	void SetupSubroutineCalls(const GraphElements::NodePtr& node);
	template<typename T> void SetupSubroutineCallsImpl(const T& node);
	void SetupSubroutineCallsImpl(const GraphElements::Alternatives& node);
	void SetupSubroutineCallsImpl(const GraphElements::LookAhead& node);
	void SetupSubroutineCallsImpl(const GraphElements::ConditionalElement& node);
	void SetupSubroutineCallsImpl(const GraphElements::SequenceCounterReset& node);
	void SetupSubroutineCallsImpl(const GraphElements::SequenceCounter& node);
	void SetupSubroutineCallsImpl(const GraphElements::NextWeakNode&);
	void SetupSubroutineCallsImpl(const GraphElements::PossessiveSequence& node);
	void SetupSubroutineCallsImpl(const GraphElements::AtomicGroup& node);
	void SetupSubroutineCallsImpl(const GraphElements::SubroutineEnter& node);
	void SetupSubroutineCallsImpl(const GraphElements::SubroutineLeave& node);

private:
	GroupStats group_stats_;
	std::unordered_map<size_t, GraphElements::NodePtr> group_nodes_;
};

RegexGraphBuildResult RegexGraphBuilder::BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	CollectGroupInternalsForRegexChain(regex_chain, group_stats_[0]);
	CollectGroupStatsForRegexChain(regex_chain, group_stats_);
	SearchRecursiveGroupCalls(group_stats_);

	GraphElements::NodePtr root;
	if(group_stats_.at(0).indirect_call_count == 0)
		root= BuildRegexGraphChain(nullptr, regex_chain);
	else
	{
		const auto end_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
		const auto node= BuildRegexGraphChain(end_node, regex_chain);
		group_nodes_[0]= node;
		root= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{nullptr, node, 0u});
	}

	SetupSubroutineCalls(root);

	RegexGraphBuildResult res;
	res.root= root;
	res.group_stats.swap(group_stats_);

	group_nodes_.clear();
	return res;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr& next,  const RegexElementsChain& chain)
{
	return BuildRegexGraphChain(next,  chain.begin(), chain.end());
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr& next,  const RegexChainIterator begin, const RegexChainIterator end)
{
	if(begin == end)
		return next;

	// If this changed, "CollectGroupInternalsForRegexElement" function must be changed too!

	const RegexElementFull& element= *begin;

	const auto next_node= BuildRegexGraphChain(next,  std::next(begin), end);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
		return BuildRegexGraphNode(next_node, element.el);
	else if(element.seq.mode == SequenceMode::Possessive)
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					BuildRegexGraphNode(nullptr, element.el),
					element.seq.min_elements,
					element.seq.max_elements,
					});
	else if(element.seq.min_elements == 0 && element.seq.max_elements == 1)
	{
		// Implement optional element using alternatives node.
		GraphElements::Alternatives alternatives;

		const auto node= BuildRegexGraphNode(next_node, element.el);
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
		const auto node= BuildRegexGraphNode(alternatives_node, element.el);
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
		const auto node= BuildRegexGraphNode(alternatives_node_node_weak, element.el);

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
		const auto node= BuildRegexGraphNode(sequence_counter_block_weak, element.el);

		std::get<GraphElements::SequenceCounter>(*sequence_counter_block).next_iteration= node;

		return std::make_shared<GraphElements::Node>(GraphElements::SequenceCounterReset{sequence_counter_block, id});
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNode(const GraphElements::NodePtr& next, const RegexElementFull::ElementType& element)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(next, el); }, element);
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const AnySymbol&)
{
	return std::make_shared<GraphElements::Node>(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const SpecificSymbol& specific_symbol)
{
	if(next != nullptr)
	{
		// Combine sequences of symbols into strings, because matcher generator generates more optimal code for strings rather than for chains of symbols.
		if(const auto next_specific_symbol= std::get_if<GraphElements::SpecificSymbol>(next.get()))
		{
			GraphElements::String string;
			string.str.push_back(specific_symbol.code);
			string.str.push_back(next_specific_symbol->code);
			string.next= next_specific_symbol->next;
			return std::make_shared<GraphElements::Node>(std::move(string));
		}
		else if(const auto next_string= std::get_if<GraphElements::String>(next.get()))
		{
			GraphElements::String string;
			string.str.push_back(specific_symbol.code);
			string.str+= next_string->str;
			string.next= next_string->next;
			return std::make_shared<GraphElements::Node>(std::move(string));
		}
	}
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next,  specific_symbol.code});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const OneOf& one_of)
{
	GraphElements::OneOf out_node;
	out_node.next= next;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Group& group)
{
	const GroupStat& stat= group_stats_.at(group.index);

	// TODO -if building regexpr for groups extraction, always emit GroupStart/GroupEnd.

	if(stat.indirect_call_count == 0)
	{
		if(stat.backreference_count == 0)
		{
			// No calls, no backreferences - generate only node for internal elements.
			return BuildRegexGraphChain(next, group.elements);
		}
		else
		{
			// Have backreferences - generate start/end nodes.
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next,  group.index});
			const auto group_contents= BuildRegexGraphChain(group_end, group.elements);
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
			const auto group_contents= BuildRegexGraphChain(subroutine_leave, group.elements);
			group_nodes_[group.index]= group_contents;
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next,  group_contents, group.index});
		}
		else
		{
			// Both backreferences and indirect calls needed - generated all nodes.

			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});

			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{subroutine_leave, group.index});
			const auto group_contents= BuildRegexGraphChain(group_end, group.elements);
			const auto group_start= std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
			group_nodes_[group.index]= group_start;
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next,  group_start, group.index});
		}
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const BackReference& back_reference)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{next,  back_reference.index});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const NonCapturingGroup& non_capturing_group)
{
	return BuildRegexGraphChain(next, non_capturing_group.elements);
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const AtomicGroup& atomic_group)
{
	return
		std::make_shared<GraphElements::Node>(
			GraphElements::AtomicGroup{
				next,
				BuildRegexGraphChain(nullptr, atomic_group.elements)});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Alternatives& alternatives)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphChain(next, alternative));

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Look& look)
{
	const auto look_graph= BuildRegexGraphChain(nullptr, look.elements);
	if(look.forward)
	{
		GraphElements::LookAhead out_node;
		out_node.next= next;
		out_node.look_graph= look_graph;
		out_node.positive= look.positive;

		return std::make_shared<GraphElements::Node>(std::move(out_node));
	}
	else
	{
		GraphElements::LookBehind out_node;
		out_node.next= next;
		out_node.look_graph= look_graph;
		out_node.positive= look.positive;
		out_node.size= GetRegexChainSize(look.elements).first; // TODO - raise error is size is not exact. Now - just use minimum size.

		return std::make_shared<GraphElements::Node>(std::move(out_node));
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const ConditionalElement& conditional_element)
{
	GraphElements::ConditionalElement out_node;
	out_node.condition_node= BuildRegexGraphNodeImpl(nullptr, conditional_element.look);
	out_node.next_true=  BuildRegexGraphChain(next, conditional_element.alternatives.alternatives[0]);
	out_node.next_false= BuildRegexGraphChain(next, conditional_element.alternatives.alternatives[1]);

	return std::make_shared<GraphElements::Node>(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const SubroutineCall& subroutine_call)
{
	// We need to save and restore state in subroutine calls.

	const GroupStat& stat= group_stats_.at(subroutine_call.index);

	// Save sequences only if recursive calls possible.
	GraphElements::SequenceIdSet sequences;
	if(stat.recursive)
		sequences= stat.internal_sequences;

	// Save group state itself and state of its subgroups, but only if they used in backreferences.
	// TODO - save groups also if we match subexpressions.
	GroupIdSet groups;

	for(const size_t& internal_group_id : stat.internal_groups)
		if(group_stats_.at(internal_group_id).backreference_count > 0)
			groups.insert(internal_group_id);

	if(group_stats_.at(subroutine_call.index).backreference_count > 0)
		groups.insert(subroutine_call.index);

	if(sequences.empty() && groups.empty())
		return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next,  nullptr /*Set actual pointer value later*/, subroutine_call.index});
	else
	{
		const auto state_restore= std::make_shared<GraphElements::Node>(GraphElements::StateRestore{next,  sequences, groups});

		const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{state_restore, nullptr /*Set actual pointer value later*/, subroutine_call.index});
		return std::make_shared<GraphElements::Node>(GraphElements::StateSave{enter_node, sequences, groups});
	}
}

void RegexGraphBuilder::SetupSubroutineCalls(const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return;

	if(const auto subroutine_enter= std::get_if<GraphElements::SubroutineEnter>(node.get()))
		if(subroutine_enter->subroutine_node == nullptr)
			subroutine_enter->subroutine_node=
				std::make_shared<GraphElements::Node>(
					GraphElements::NextWeakNode{group_nodes_.at(subroutine_enter->index)});

	std::visit([&](const auto& el){ return SetupSubroutineCallsImpl(el); }, *node);
}

template<typename T>
void RegexGraphBuilder::SetupSubroutineCallsImpl(const T& node)
{
	SetupSubroutineCalls(node.next);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::Alternatives& node)
{
	for(const auto& next : node.next)
		SetupSubroutineCalls(next);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::LookAhead& node)
{
	SetupSubroutineCalls(node.next);
	SetupSubroutineCalls(node.look_graph);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::ConditionalElement& node)
{
	SetupSubroutineCalls(node.condition_node);
	SetupSubroutineCalls(node.next_true);
	SetupSubroutineCalls(node.next_false);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::SequenceCounterReset& node)
{
	SetupSubroutineCalls(node.next);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::SequenceCounter& node)
{
	SetupSubroutineCalls(node.next_iteration);
	SetupSubroutineCalls(node.next_sequence_end);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::NextWeakNode&){}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::PossessiveSequence& node)
{
	SetupSubroutineCalls(node.next);
	SetupSubroutineCalls(node.sequence_element);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::AtomicGroup& node)
{
	SetupSubroutineCalls(node.next);
	SetupSubroutineCalls(node.group_element);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::SubroutineEnter& node)
{
	SetupSubroutineCalls(node.subroutine_node);
	SetupSubroutineCalls(node.next);
}

void RegexGraphBuilder::SetupSubroutineCallsImpl(const GraphElements::SubroutineLeave&){}

} // namespace

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	RegexGraphBuilder builder;
	return builder.BuildRegexGraph(regex_chain);
}

} // namespace RegPanzer
