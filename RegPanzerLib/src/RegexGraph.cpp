#include "../RegexGraph.hpp"
#include "../Utils.hpp"
#include <cassert>
#include <optional>
#include <unordered_map>

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

void CollectGroupInternalsForElementImpl(const LineStartAssertion&, GroupStat&){}
void CollectGroupInternalsForElementImpl(const LineEndAssertion&, GroupStat&){}

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

void CollectGroupStatsForElementImpl(const LineStartAssertion&, GroupStats&){}
void CollectGroupStatsForElementImpl(const LineEndAssertion&, GroupStats&){}

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

// Size in UTF-8 bytes.
using MinMaxSize= std::pair<size_t, size_t>;

MinMaxSize GetRegexChainSize(const RegexElementsChain& regex_chain);

MinMaxSize GetRegexElementSize_impl(const AnySymbol&)
{
	// All sizes are possible for any symbol.
	// TODO - maybe process just raw bytes in "any" symbol?
	return MinMaxSize{1, 6};
}

MinMaxSize GetRegexElementSize_impl(const SpecificSymbol& specific_symbol)
{
	const CharType str_utf32[]{specific_symbol.code, 0};
	const size_t size= Utf32ToUtf8(str_utf32).size();
	return MinMaxSize{size, size};
}

MinMaxSize GetRegexElementSize_impl(const OneOf& one_of)
{
	if(one_of.inverse_flag)
		return MinMaxSize{1, 6};

	MinMaxSize res{100, 0};

	for(const CharType c : one_of.variants)
	{
		const CharType str_utf32[]{c, 0};
		const size_t size= Utf32ToUtf8(str_utf32).size();
		res.first = std::min(res.first , size);
		res.second= std::max(res.second, size);
	}

	for(const auto& range : one_of.ranges)
	{
		const CharType begin_str_utf32[]{range.first , 0};
		const CharType   end_str_utf32[]{range.second, 0};
		const size_t min_size= Utf32ToUtf8(begin_str_utf32).size();
		const size_t max_size= Utf32ToUtf8(  end_str_utf32).size();
		res.first = std::min(res.first , min_size);
		res.second= std::max(res.second, max_size);
	}

	return res;
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

MinMaxSize GetRegexElementSize_impl(const LineStartAssertion&)
{
	return MinMaxSize{0, 0};
}

MinMaxSize GetRegexElementSize_impl(const LineEndAssertion&)
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
	explicit RegexGraphBuilder(const Options& options);

	RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain);

private:
	using RegexChainIterator= RegexElementsChain::const_iterator;

	GraphElements::NodePtr BuildRegexGraphChain(GraphElements::NodePtr next, const RegexElementsChain& chain);
	GraphElements::NodePtr BuildRegexGraphChain(GraphElements::NodePtr next, RegexChainIterator begin, RegexChainIterator end);

	GraphElements::NodePtr BuildRegexGraphNode(GraphElements::NodePtr next, const RegexElementFull::ElementType& element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const AnySymbol&);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const SpecificSymbol& specific_symbol);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const OneOf& one_of);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const Group& group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const BackReference& back_reference);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const NonCapturingGroup& non_capturing_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const AtomicGroup& atomic_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const Alternatives& alternatives);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const Look& look);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const LineStartAssertion& line_start_assertion);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const LineEndAssertion& line_end_assertion);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const ConditionalElement& conditional_element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(GraphElements::NodePtr next, const SubroutineCall& subroutine_call);

	void SetupSubroutineCalls();

private:
	const Options options_;
	GroupStats group_stats_;
	// Collect list of sequence counters to use it later to prevent unnecessary state save/restore generation.
	GraphElements::SequenceIdSet used_sequence_counters_;
	// Collect group enter and subroutine enter nodes to setup pointers to subroutine calls later.
	std::unordered_map<size_t, GraphElements::NodePtr> group_nodes_;
	std::unordered_map<size_t, std::vector<GraphElements::NodePtr>> subroutine_enter_nodes_;
	GraphElements::NodesStorage nodes_storage_;
};

RegexGraphBuilder::RegexGraphBuilder(const Options& options)
	: options_(options)
{
}

RegexGraphBuildResult RegexGraphBuilder::BuildRegexGraph(const RegexElementsChain& regex_chain)
{
	CollectGroupInternalsForRegexChain(regex_chain, group_stats_[0]);
	CollectGroupStatsForRegexChain(regex_chain, group_stats_);
	SearchRecursiveGroupCalls(group_stats_);

	GraphElements::NodePtr root= nullptr;
	if(group_stats_.at(0).indirect_call_count == 0)
		root= BuildRegexGraphChain(nullptr, regex_chain);
	else
	{
		const auto end_node= nodes_storage_.Allocate(GraphElements::SubroutineLeave{});
		const auto node= BuildRegexGraphChain(end_node, regex_chain);
		group_nodes_[0]= node;
		root= nodes_storage_.Allocate(GraphElements::SubroutineEnter{nullptr, node, 0u});
	}

	SetupSubroutineCalls();

	RegexGraphBuildResult res;
	res.options= options_;
	res.root= root;
	res.group_stats.swap(group_stats_);
	res.used_sequence_counters.swap(used_sequence_counters_);
	std::swap(res.nodes_storage, nodes_storage_);

	group_nodes_.clear();
	subroutine_enter_nodes_.clear();
	return res;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr next, const RegexElementsChain& chain)
{
	return BuildRegexGraphChain(next, chain.begin(), chain.end());
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr next, const RegexChainIterator begin, const RegexChainIterator end)
{
	if(begin == end)
		return next;

	// If this changed, "CollectGroupInternalsForRegexElement" function must be changed too!

	const RegexElementFull& element= *begin;

	const auto next_node= BuildRegexGraphChain(next, std::next(begin), end);

	const auto node_possessive= BuildRegexGraphNode(nullptr, element.el);

	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
		return BuildRegexGraphNode(next_node, element.el);
	else if(element.seq.mode == SequenceMode::Possessive)
	{
		return
			nodes_storage_.Allocate(
				GraphElements::PossessiveSequence{
					next_node,
					node_possessive,
					element.seq.min_elements,
					element.seq.max_elements,
					});
	}
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

		return nodes_storage_.Allocate(std::move(alternatives));
	}
	else if(element.seq.min_elements == 1 && element.seq.max_elements == Sequence::c_max)
	{
		// In case of one or more elemenst first enter sequence body node, then alternatives node.

		const auto alternatives_node= nodes_storage_.Allocate(GraphElements::Alternatives{{next_node}});
		const auto node= BuildRegexGraphNode(alternatives_node, element.el);

		auto& alternatives= std::get<GraphElements::Alternatives>(*alternatives_node);
		if(element.seq.mode == SequenceMode::Lazy)
			alternatives.next.push_back(node);
		else
			alternatives.next.insert(alternatives.next.begin(), node);

		return node;
	}
	else if(element.seq.min_elements == 0 && element.seq.max_elements == Sequence::c_max)
	{
		// In case of zero or more elements first enter alternatives node, than sequence body node.

		const auto alternatives_node= nodes_storage_.Allocate(GraphElements::Alternatives{{next_node}});
		const auto node= BuildRegexGraphNode(alternatives_node, element.el);

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

		used_sequence_counters_.insert(id);

		const auto sequence_counter_block=
			nodes_storage_.Allocate(
				GraphElements::SequenceCounter{
					nullptr,
					next_node,
					id,
					element.seq.min_elements,
					element.seq.max_elements,
					element.seq.mode != SequenceMode::Lazy,
					});

		const auto node= BuildRegexGraphNode(sequence_counter_block, element.el);

		std::get<GraphElements::SequenceCounter>(*sequence_counter_block).next_iteration= node;

		return nodes_storage_.Allocate(GraphElements::SequenceCounterReset{sequence_counter_block, id});
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNode(const GraphElements::NodePtr next, const RegexElementFull::ElementType& element)
{
	return std::visit([&](const auto& el){ return BuildRegexGraphNodeImpl(next, el); }, element);
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const AnySymbol&)
{
	return nodes_storage_.Allocate(GraphElements::AnySymbol{next});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const SpecificSymbol& specific_symbol)
{
	return nodes_storage_.Allocate(GraphElements::SpecificSymbol{next, specific_symbol.code});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const OneOf& one_of)
{
	GraphElements::OneOf out_node;
	out_node.next= next;
	out_node.variants= one_of.variants;
	out_node.ranges= one_of.ranges;
	out_node.inverse_flag= one_of.inverse_flag;
	return nodes_storage_.Allocate(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const Group& group)
{
	const GroupStat& stat= group_stats_.at(group.index);

	if(stat.indirect_call_count == 0)
	{
		if(stat.backreference_count == 0 && !options_.extract_groups)
		{
			// No calls, no backreferences - generate only node for internal elements.
			return BuildRegexGraphChain(next, group.elements);
		}
		else
		{
			// Have backreferences - generate start/end nodes.
			const auto group_end= nodes_storage_.Allocate(GraphElements::GroupEnd{next, group.index});
			const auto group_contents= BuildRegexGraphChain(group_end, group.elements);
			return nodes_storage_.Allocate(GraphElements::GroupStart{group_contents, group.index});
		}
	}
	else
	{
		// We do not need to save state here, save will be saved (if needed) in indirect call.

		if(stat.backreference_count == 0 && !options_.extract_groups)
		{
			// No backreferences - generate only enter/leave nodes.
			const auto subroutine_leave= nodes_storage_.Allocate(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(subroutine_leave, group.elements);
			group_nodes_[group.index]= group_contents;
			return nodes_storage_.Allocate(GraphElements::SubroutineEnter{next, group_contents, group.index});
		}
		else
		{
			// Both backreferences and indirect calls needed - generated all nodes.
			const auto group_end= nodes_storage_.Allocate(GraphElements::GroupEnd{next, group.index});
			const auto subroutine_leave= nodes_storage_.Allocate(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(subroutine_leave, group.elements);
			group_nodes_[group.index]= group_contents;
			const auto subroutine_enter= nodes_storage_.Allocate(GraphElements::SubroutineEnter{group_end, group_contents, group.index});
			return nodes_storage_.Allocate(GraphElements::GroupStart{subroutine_enter, group.index});
		}
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const BackReference& back_reference)
{
	return nodes_storage_.Allocate(GraphElements::BackReference{next, back_reference.index});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const NonCapturingGroup& non_capturing_group)
{
	return BuildRegexGraphChain(next, non_capturing_group.elements);
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const AtomicGroup& atomic_group)
{
	return
		nodes_storage_.Allocate(
			GraphElements::AtomicGroup{
				next,
				BuildRegexGraphChain(nullptr, atomic_group.elements)});
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const Alternatives& alternatives)
{
	GraphElements::Alternatives out_node;
	for(const auto& alternative : alternatives.alternatives)
		out_node.next.push_back(BuildRegexGraphChain(next, alternative));

	return nodes_storage_.Allocate(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const Look& look)
{
	const auto look_graph= BuildRegexGraphChain(nullptr, look.elements);
	if(look.forward)
	{
		GraphElements::LookAhead out_node;
		out_node.next= next;
		out_node.look_graph= look_graph;
		out_node.positive= look.positive;

		return nodes_storage_.Allocate(std::move(out_node));
	}
	else
	{
		GraphElements::LookBehind out_node;
		out_node.next= next;
		out_node.look_graph= look_graph;
		out_node.positive= look.positive;
		out_node.size= GetRegexChainSize(look.elements).first; // TODO - raise error is size is not exact. Now - just use minimum size.

		return nodes_storage_.Allocate(std::move(out_node));
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const LineStartAssertion& line_start_assertion)
{
	(void)line_start_assertion;

	const auto string_start_assertion_node= nodes_storage_.Allocate(GraphElements::StringStartAssertion{next});

	if(options_.multiline)
	{
		// In multiline mode check line start or string start.

		// TODO - support CR LF
		GraphElements::OneOf one_of;
		one_of.variants.push_back('\n');
		const auto one_of_node= nodes_storage_.Allocate(std::move(one_of));

		const auto look_behind_node= nodes_storage_.Allocate(GraphElements::LookBehind{next, one_of_node, true, 1});

		GraphElements::Alternatives alternatives;
		alternatives.next.push_back(string_start_assertion_node);
		alternatives.next.push_back(look_behind_node);

		return nodes_storage_.Allocate(std::move(alternatives));
	}
	else
		return string_start_assertion_node;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const LineEndAssertion& line_end_assertion)
{
	(void)line_end_assertion;

	const auto string_end_assertion_node= nodes_storage_.Allocate(GraphElements::StringEndAssertion{next});

	if(options_.multiline)
	{
		// In multiline mode check line end or string end.

		// TODO - support CR LF

		GraphElements::OneOf one_of;
		one_of.variants.push_back('\n');
		const auto one_of_node= nodes_storage_.Allocate(std::move(one_of));

		const auto look_ahead_node= nodes_storage_.Allocate(GraphElements::LookAhead{next, one_of_node, true});

		GraphElements::Alternatives alternatives;
		alternatives.next.push_back(string_end_assertion_node);
		alternatives.next.push_back(look_ahead_node);

		return nodes_storage_.Allocate(std::move(alternatives));
	}

	return string_end_assertion_node;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const ConditionalElement& conditional_element)
{
	GraphElements::ConditionalElement out_node;
	out_node.condition_node= BuildRegexGraphNodeImpl(nullptr, conditional_element.look);
	out_node.next_true=  BuildRegexGraphChain(next, conditional_element.alternatives.alternatives[0]);
	out_node.next_false= BuildRegexGraphChain(next, conditional_element.alternatives.alternatives[1]);

	return nodes_storage_.Allocate(std::move(out_node));
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr next, const SubroutineCall& subroutine_call)
{
	// We need to save and restore state in subroutine calls.

	const GroupStat& stat= group_stats_.at(subroutine_call.index);

	// Save sequences only if recursive calls possible.
	GraphElements::SequenceIdSet sequences;
	if(stat.recursive)
		sequences= stat.internal_sequences;

	// Save state of group's subgroups, but only if they used in backreferences or if groups extraction is enabled.
	GroupIdSet groups;

	for(const size_t& internal_group_id : stat.internal_groups)
		if(options_.extract_groups || group_stats_.at(internal_group_id).backreference_count > 0)
			groups.insert(internal_group_id);

	const GraphElements::NodePtr subroutine_node= nullptr; // Set actual pointer value later.
	if(sequences.empty() && groups.empty())
	{
		const auto enter_node= nodes_storage_.Allocate(GraphElements::SubroutineEnter{next, subroutine_node, subroutine_call.index});
		subroutine_enter_nodes_[subroutine_call.index].push_back(enter_node);
		return enter_node;
	}
	else
	{
		const auto state_restore= nodes_storage_.Allocate(GraphElements::StateRestore{next, sequences, groups});
		const auto enter_node= nodes_storage_.Allocate(GraphElements::SubroutineEnter{state_restore, subroutine_node, subroutine_call.index});
		subroutine_enter_nodes_[subroutine_call.index].push_back(enter_node);
		return nodes_storage_.Allocate(GraphElements::StateSave{enter_node, sequences, groups});
	}
}

void RegexGraphBuilder::SetupSubroutineCalls()
{
	for(const auto& subroutine_call_pair : subroutine_enter_nodes_)
	{
		for(const GraphElements::NodePtr node_ptr : subroutine_call_pair.second)
		{
			const auto subroutine_enter= std::get_if<GraphElements::SubroutineEnter>(node_ptr);
			assert(subroutine_enter != nullptr);
			subroutine_enter->subroutine_node= group_nodes_.at(subroutine_call_pair.first);
		}
	}
}

} // namespace

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain, const Options& options)
{
	RegexGraphBuilder builder(options);
	return builder.BuildRegexGraph(regex_chain);
}

} // namespace RegPanzer
