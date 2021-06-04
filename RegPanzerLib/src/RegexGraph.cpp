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
// Symbols set stuff
//

OneOf CombineSymbolSets(const OneOf& l, const OneOf& r)
{
	if(l.inverse_flag || r.inverse_flag) // TODO - support merging inversed "OneOf"
		return OneOf{ {}, {}, true };

	OneOf res;
	res.variants.insert(res.variants.end(), l.variants.begin(), l.variants.end());
	res.ranges.insert(res.ranges.end(), l.ranges.begin(), l.ranges.end());
	res.variants.insert(res.variants.end(), r.variants.begin(), r.variants.end());
	res.ranges.insert(res.ranges.end(), r.ranges.begin(), r.ranges.end());
	return res;
}

// May return "true" even if symbols set actually are not intersected.
bool HasIntersection(const OneOf& l, const OneOf& r)
{
	if(l.inverse_flag || r.inverse_flag)
		return true; // TODO - process such case more precisely.

	// Check variant againt variant.
	for(const CharType l_char : l.variants)
	for(const CharType r_char : r.variants)
		if(l_char == r_char)
			return true;

	// Check ranges against ranges.
	for(const auto& l_range : l.ranges)
	for(const auto& r_range : r.ranges)
		if(!(l_range.second < r_range.first || l_range.first > r_range.second))
			return true;

	// Check variants against ranges.
	for(const CharType l_char : l.variants)
	for(const auto& r_range : r.ranges)
		if(l_char >= r_range.first && l_char <= r_range.second)
			return true;

	// Check ranges against variants.
	for(const auto& l_range : l.ranges)
	for(const CharType r_char : r.variants)
		if(r_char >= l_range.first && r_char <= l_range.second)
			return true;

	return false;
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

	GraphElements::NodePtr BuildRegexGraphChain(const GraphElements::NodePtr& next, const RegexElementsChain& chain);
	GraphElements::NodePtr BuildRegexGraphChain(const GraphElements::NodePtr& next, RegexChainIterator begin, RegexChainIterator end);

	GraphElements::NodePtr BuildRegexGraphNode(const GraphElements::NodePtr& next, const RegexElementFull::ElementType& element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const AnySymbol&);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const SpecificSymbol& specific_symbol);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const OneOf& one_of);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Group& group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const BackReference& back_reference);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const NonCapturingGroup& non_capturing_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const AtomicGroup& atomic_group);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Alternatives& alternatives);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const Look& look);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const LineStartAssertion& line_start_assertion);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const LineEndAssertion& line_end_assertion);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const ConditionalElement& conditional_element);
	GraphElements::NodePtr BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const SubroutineCall& subroutine_call);

	void SetupSubroutineCalls();

	OneOf GetPossibleStartSybmols(const GraphElements::NodePtr& node);

	OneOf GetPossibleStartSybmolsImpl(const GraphElements::AnySymbol& any_symbol);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::SpecificSymbol& specific_symbol);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::String& string);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::OneOf& one_of);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::Alternatives& alternatives);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::GroupStart& group_start);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::GroupEnd& group_end);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::BackReference& back_reference);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::LookAhead& look_ahead);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::LookBehind& look_behind);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::StringStartAssertion& string_start_assertion);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::StringEndAssertion& string_end_assertion);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::ConditionalElement& conditional_element);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounterReset& sequence_counter_reset);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounter& sequence_counter);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::NextWeakNode& next_weak);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::PossessiveSequence& possessive_sequence);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::AtomicGroup& atomic_group);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::SubroutineEnter& subroutine_enter);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::SubroutineLeave& subroutine_leave);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::StateSave& state_save);
	OneOf GetPossibleStartSybmolsImpl(const GraphElements::StateRestore& state_restore);

	using SizeOpt= std::optional<size_t>;
	// Returns non-empty value if element is allowed inside fixed length element sequence.
	SizeOpt GetFixedElementSize(const GraphElements::NodePtr& node);

	SizeOpt GetFixedElementSizeImpl(const GraphElements::AnySymbol& any_symbol);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::SpecificSymbol& specific_symbol);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::String& string);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::OneOf& one_of);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::Alternatives& alternatives);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::GroupStart& group_start);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::GroupEnd& group_end);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::BackReference& back_reference);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::LookAhead& look_ahead);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::LookBehind& look_behind);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::StringStartAssertion& string_start_assertion);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::StringEndAssertion& string_end_assertion);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::ConditionalElement& conditional_element);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::SequenceCounterReset& sequence_counter_reset);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::SequenceCounter& sequence_counter);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::NextWeakNode& next_weak);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::PossessiveSequence& possessive_sequence);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::AtomicGroup& atomic_group);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::SubroutineEnter& subroutine_enter);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::SubroutineLeave& subroutine_leave);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::StateSave& state_save);
	SizeOpt GetFixedElementSizeImpl(const GraphElements::StateRestore& state_restore);

private:
	const Options options_;
	GroupStats group_stats_;
	// Collect list of sequence counters to use it later to prevent unnecessary state save/restore generation.
	GraphElements::SequenceIdSet used_sequence_counters_;
	// Collect group enter and subroutine enter nodes to setup pointers to subroutine calls later.
	std::unordered_map<size_t, GraphElements::NodePtr> group_nodes_;
	std::unordered_map<size_t, std::vector<GraphElements::NodePtr>> subroutine_enter_nodes_;
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

	SetupSubroutineCalls();

	RegexGraphBuildResult res;
	res.options= options_;
	res.root= root;
	res.group_stats.swap(group_stats_);
	res.used_sequence_counters.swap(used_sequence_counters_);

	group_nodes_.clear();
	subroutine_enter_nodes_.clear();
	return res;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr& next, const RegexElementsChain& chain)
{
	return BuildRegexGraphChain(next, chain.begin(), chain.end());
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphChain(const GraphElements::NodePtr& next, const RegexChainIterator begin, const RegexChainIterator end)
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
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					node_possessive,
					element.seq.min_elements,
					element.seq.max_elements,
					});
	}
	/* Auto-possessification optimization.
		Sequence may be converted into possessive if there is no way to match expression returning back to previous sequence element.
		It is true if there is no way to match expression after sequence and sequence element simultaniously.
		Here we check this by collecting set of start symbols for sequence element and for element after sequence.
		If there is no itersection between two sets of symbols - apply auto-possessification.
	*/
	else if(
		element.seq.mode != SequenceMode::Lazy &&
		!HasIntersection(
			GetPossibleStartSybmols(node_possessive),
			GetPossibleStartSybmols(next_node)))
	{
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::PossessiveSequence{
					next_node,
					node_possessive,
					element.seq.min_elements,
					element.seq.max_elements,
					});
	}
	/*
	 * Try to perform fixed length element sequence optimization.
	 * For such optimization sequence element should have fixed size and should not modify state except position pointer (sequence counters, groups, etc.).
	*/
	else if(
		const auto fixed_length_element_size= GetFixedElementSize(node_possessive);
		fixed_length_element_size != std::nullopt && element.seq.mode == SequenceMode::Greedy)
	{
		return
			std::make_shared<GraphElements::Node>(
				GraphElements::FixedLengthElementSequence{
					next_node,
					node_possessive,
					element.seq.min_elements,
					element.seq.max_elements,
					*fixed_length_element_size,
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

		used_sequence_counters_.insert(id);

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

			const CharType str_utf32[]{specific_symbol.code, next_specific_symbol->code, 0};
			string.str= Utf32ToUtf8(str_utf32);

			string.next= next_specific_symbol->next;
			return std::make_shared<GraphElements::Node>(std::move(string));
		}
		else if(const auto next_string= std::get_if<GraphElements::String>(next.get()))
		{
			GraphElements::String string;

			const CharType str_utf32[]{specific_symbol.code, 0};
			string.str= Utf32ToUtf8(str_utf32);
			string.str+= next_string->str;

			string.next= next_string->next;
			return std::make_shared<GraphElements::Node>(std::move(string));
		}
	}
	return std::make_shared<GraphElements::Node>(GraphElements::SpecificSymbol{next, specific_symbol.code});
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
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next, group.index});
			const auto group_contents= BuildRegexGraphChain(group_end, group.elements);
			return std::make_shared<GraphElements::Node>(GraphElements::GroupStart{group_contents, group.index});
		}
	}
	else
	{
		// We do not need to save state here, save will be saved (if needed) in indirect call.

		if(stat.backreference_count == 0 && !options_.extract_groups)
		{
			// No backreferences - generate only enter/leave nodes.
			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(subroutine_leave, group.elements);
			group_nodes_[group.index]= group_contents;
			return std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, group_contents, group.index});
		}
		else
		{
			// Both backreferences and indirect calls needed - generated all nodes.
			const auto group_end= std::make_shared<GraphElements::Node>(GraphElements::GroupEnd{next, group.index});
			const auto subroutine_leave= std::make_shared<GraphElements::Node>(GraphElements::SubroutineLeave{});
			const auto group_contents= BuildRegexGraphChain(subroutine_leave, group.elements);
			group_nodes_[group.index]= group_contents;
			const auto subroutine_enter= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{group_end, group_contents, group.index});
			return std::make_shared<GraphElements::Node>(GraphElements::GroupStart{subroutine_enter, group.index});
		}
	}
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const BackReference& back_reference)
{
	return std::make_shared<GraphElements::Node>(GraphElements::BackReference{next, back_reference.index});
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

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const LineStartAssertion& line_start_assertion)
{
	(void)line_start_assertion;

	const auto string_start_assertion_node= std::make_shared<GraphElements::Node>(GraphElements::StringStartAssertion{next});

	if(options_.multiline)
	{
		// In multiline mode check line start or string start.

		// TODO - support CR LF
		GraphElements::OneOf one_of;
		one_of.variants.push_back('\n');
		const auto one_of_node= std::make_shared<GraphElements::Node>(std::move(one_of));

		const auto look_behind_node= std::make_shared<GraphElements::Node>(GraphElements::LookBehind{next, one_of_node, true, 1});

		GraphElements::Alternatives alternatives;
		alternatives.next.push_back(string_start_assertion_node);
		alternatives.next.push_back(look_behind_node);

		return std::make_shared<GraphElements::Node>(std::move(alternatives));
	}
	else
		return string_start_assertion_node;
}

GraphElements::NodePtr RegexGraphBuilder::BuildRegexGraphNodeImpl(const GraphElements::NodePtr& next, const LineEndAssertion& line_end_assertion)
{
	(void)line_end_assertion;

	const auto string_end_assertion_node= std::make_shared<GraphElements::Node>(GraphElements::StringEndAssertion{next});

	if(options_.multiline)
	{
		// In multiline mode check line end or string end.

		// TODO - support CR LF

		GraphElements::OneOf one_of;
		one_of.variants.push_back('\n');
		const auto one_of_node= std::make_shared<GraphElements::Node>(std::move(one_of));

		const auto look_ahead_node= std::make_shared<GraphElements::Node>(GraphElements::LookAhead{next, one_of_node, true});

		GraphElements::Alternatives alternatives;
		alternatives.next.push_back(string_end_assertion_node);
		alternatives.next.push_back(look_ahead_node);

		return std::make_shared<GraphElements::Node>(std::move(alternatives));
	}

	return string_end_assertion_node;
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

	// Save state of group's subgroups, but only if they used in backreferences or if groups extraction is enabled.
	GroupIdSet groups;

	for(const size_t& internal_group_id : stat.internal_groups)
		if(options_.extract_groups || group_stats_.at(internal_group_id).backreference_count > 0)
			groups.insert(internal_group_id);

	const GraphElements::NodePtr subroutine_node= nullptr; // Set actual pointer value later.
	if(sequences.empty() && groups.empty())
	{
		const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{next, subroutine_node, subroutine_call.index});
		subroutine_enter_nodes_[subroutine_call.index].push_back(enter_node);
		return enter_node;
	}
	else
	{
		const auto state_restore= std::make_shared<GraphElements::Node>(GraphElements::StateRestore{next, sequences, groups});
		const auto enter_node= std::make_shared<GraphElements::Node>(GraphElements::SubroutineEnter{state_restore, subroutine_node, subroutine_call.index});
		subroutine_enter_nodes_[subroutine_call.index].push_back(enter_node);
		return std::make_shared<GraphElements::Node>(GraphElements::StateSave{enter_node, sequences, groups});
	}
}

void RegexGraphBuilder::SetupSubroutineCalls()
{
	for(const auto& subroutine_call_pair : subroutine_enter_nodes_)
	{
		for(const GraphElements::NodePtr& node_ptr : subroutine_call_pair.second)
		{
			const auto subroutine_enter= std::get_if<GraphElements::SubroutineEnter>(node_ptr.get());
			assert(subroutine_enter != nullptr);
			// Use weak pointers for indirect calls to prevent strong loops.
			subroutine_enter->subroutine_node=
				std::make_shared<GraphElements::Node>(
					GraphElements::NextWeakNode{group_nodes_.at(subroutine_call_pair.first)});
		}
	}
}

OneOf RegexGraphBuilder::GetPossibleStartSybmols(const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return OneOf{}; // Empty set of symbols.

	return std::visit([&](const auto& el){ return GetPossibleStartSybmolsImpl(el); }, *node);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::AnySymbol& any_symbol)
{
	(void)any_symbol;
	// Inverted empty set.
	// TODO - process "AnySymbol" with exclusions.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::SpecificSymbol& specific_symbol)
{
	return OneOf{ {specific_symbol.code}, {}, false };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::String& string)
{
	const auto str_utf32= Utf8ToUtf32(string.str);
	if(!str_utf32.empty())
		return OneOf{ {str_utf32.front()}, {}, false };
	return GetPossibleStartSybmols(string.next);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::OneOf& one_of)
{
	return OneOf{ one_of.variants, one_of.ranges, one_of.inverse_flag };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::Alternatives& alternatives)
{
	OneOf res;
	for(const GraphElements::NodePtr& next : alternatives.next)
		res= CombineSymbolSets(res, GetPossibleStartSybmols(next));

	return res;
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::GroupStart& group_start)
{
	return GetPossibleStartSybmols(group_start.next);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::GroupEnd& group_end)
{
	return GetPossibleStartSybmols(group_end.next);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::BackReference& back_reference)
{
	(void)back_reference;
	// Any symbol is possible in backreference.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::LookAhead& look_ahead)
{
	(void)look_ahead;

	// Any symbol is possible in look_ahead.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::LookBehind& look_behind)
{
	(void)look_behind;

	// Any symbol is possible in look_behind.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::StringStartAssertion& string_start_assertion)
{
	(void)string_start_assertion;

	// Fail possessification optimization in such case - return all possible symbols.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::StringEndAssertion& string_end_assertion)
{
	(void)string_end_assertion;

	// Fail possessification optimization in such case - return all possible symbols.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::ConditionalElement& conditional_element)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(conditional_element.next_true),
		GetPossibleStartSybmols(conditional_element.next_false));
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounterReset& sequence_counter_reset)
{
	return GetPossibleStartSybmols(sequence_counter_reset.next);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounter& sequence_counter)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(sequence_counter.next_iteration),
		GetPossibleStartSybmols(sequence_counter.next_sequence_end));
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::NextWeakNode& next_weak)
{
	return GetPossibleStartSybmols(next_weak.next.lock());
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::PossessiveSequence& possessive_sequence)
{
	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(possessive_sequence.sequence_element),
		GetPossibleStartSybmols(possessive_sequence.next));
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence)
{
	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(fixed_length_element_sequence.sequence_element),
		GetPossibleStartSybmols(fixed_length_element_sequence.next));
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::AtomicGroup& atomic_group)
{
	// Combine for cases of empty atomic group. TODO - is this really needed? Or maybe symbols set will contain all symbols in empty group?
	return CombineSymbolSets(
		GetPossibleStartSybmols(atomic_group.next),
		GetPossibleStartSybmols(atomic_group.group_element));
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::SubroutineEnter& subroutine_enter)
{
	return GetPossibleStartSybmols(subroutine_enter.subroutine_node);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::SubroutineLeave& subroutine_leave)
{
	(void)subroutine_leave;

	// Any symbol is possible after subroutine leave.
	return OneOf{ {}, {}, true };
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::StateSave& state_save)
{
	return GetPossibleStartSybmols(state_save.next);
}

OneOf RegexGraphBuilder::GetPossibleStartSybmolsImpl(const GraphElements::StateRestore& state_restore)
{
	return GetPossibleStartSybmols(state_restore.next);
}

std::optional<size_t> RegexGraphBuilder::GetFixedElementSize(const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return size_t(0);

	return std::visit([&](const auto& el){ return GetFixedElementSizeImpl(el); }, *node);
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::AnySymbol&)
{
	// Any symbol has non-fixed size.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::SpecificSymbol& specific_symbol)
{
	const auto next_size= GetFixedElementSize(specific_symbol.next);
	if(next_size == std::nullopt)
		return std::nullopt;

	const CharType str_utf32[]{specific_symbol.code, 0};
	return *next_size + Utf32ToUtf8(str_utf32).size();
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::String& string)
{
	const auto next_size= GetFixedElementSize(string.next);
	if(next_size == std::nullopt)
		return std::nullopt;

	return *next_size + string.str.size();
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::OneOf& one_of)
{
	const auto next_size= GetFixedElementSize(one_of.next);
	if(next_size == std::nullopt)
		return std::nullopt;

	if(one_of.inverse_flag) // Practically almost all sizes are possible in inverted "OneOf".
		return std::nullopt;

	// TODO - remove copy-paste.
	// TODO - what if "OneOf" is empty?
	MinMaxSize sizes{100, 0};

	for(const CharType c : one_of.variants)
	{
		const CharType str_utf32[]{c, 0};
		const size_t size= Utf32ToUtf8(str_utf32).size();
		sizes.first = std::min(sizes.first , size);
		sizes.second= std::max(sizes.second, size);
	}

	for(const auto& range : one_of.ranges)
	{
		const CharType begin_str_utf32[]{range.first , 0};
		const CharType   end_str_utf32[]{range.second, 0};
		const size_t min_size= Utf32ToUtf8(begin_str_utf32).size();
		const size_t max_size= Utf32ToUtf8(  end_str_utf32).size();
		sizes.first = std::min(sizes.first , min_size);
		sizes.second= std::max(sizes.second, max_size);
	}

	if(sizes.first != sizes.second)
		return std::nullopt;
	return *next_size + sizes.first;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::Alternatives& alternatives)
{
	SizeOpt s;
	for(const auto& next : alternatives.next)
	{
		const auto next_size= GetFixedElementSize(next);
		if(next_size == std::nullopt)
			return std::nullopt;
		if(s == std::nullopt)
			s= next_size;
		else if(*s != *next_size)
			return std::nullopt;
	}

	return s;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::GroupStart&)
{
	// Disable fixed lenght element sequence optimization if we need to capture groups inside sequence element.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::GroupEnd&)
{
	// Disable fixed lenght element sequence optimization if we need to capture groups inside sequence element.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::BackReference&)
{
	// Backreference generally has non-fixed size.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::LookAhead&)
{
	// Disable lookahead in elements of fixed length element sequences.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::LookBehind&)
{
	// Disable lookbehind in elements of fixed length element sequences.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::StringStartAssertion&)
{
	// It's probably wrong to have string position assertions in fixed length element sequence elements.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::StringEndAssertion&)
{
	// It's probably wrong to have string position assertions in fixed length element sequence elements.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::ConditionalElement&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::SequenceCounterReset&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::SequenceCounter&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::NextWeakNode&)
{
	// Such kind of nodes used only in indirect calls and in greedy sequences. So, disable such nodes inside fixed length element sequences.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::PossessiveSequence& possessive_sequence)
{
	// Enable posessive sequences inside fixed length element sequences, but only with fixed size.

	if(possessive_sequence.min_elements != possessive_sequence.max_elements)
		return std::nullopt;

	const auto next_size= GetFixedElementSize(possessive_sequence.next);
	if(next_size == std::nullopt)
		return std::nullopt;

	const auto element_size= GetFixedElementSize(possessive_sequence.sequence_element);
	if(element_size == std::nullopt)
		return std::nullopt;

	return *next_size + possessive_sequence.min_elements * *element_size;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence)
{
	if(fixed_length_element_sequence.min_elements != fixed_length_element_sequence.max_elements)
		return std::nullopt;

	const auto next_size= GetFixedElementSize(fixed_length_element_sequence.next);
	if(next_size == std::nullopt)
		return std::nullopt;

	const auto element_size= GetFixedElementSize(fixed_length_element_sequence.sequence_element);
	if(element_size == std::nullopt)
		return std::nullopt;

	return *next_size + fixed_length_element_sequence.min_elements * *element_size;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::AtomicGroup& atomic_group)
{
	return GetFixedElementSize(atomic_group.group_element);
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::SubroutineEnter&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::SubroutineLeave&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::StateSave&)
{
	// It's too complicated.
	return std::nullopt;
}

RegexGraphBuilder::SizeOpt RegexGraphBuilder::GetFixedElementSizeImpl(const GraphElements::StateRestore&)
{
	// It's too complicated.
	return std::nullopt;
}

} // namespace

RegexGraphBuildResult BuildRegexGraph(const RegexElementsChain& regex_chain, const Options& options)
{
	RegexGraphBuilder builder(options);
	return builder.BuildRegexGraph(regex_chain);
}

} // namespace RegPanzer
