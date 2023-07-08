#include "../RegexGraphOptizer.hpp"
#include "../Utils.hpp"

namespace RegPanzer
{

namespace
{

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

OneOf GetPossibleStartSybmolsImpl(const GraphElements::AnySymbol& any_symbol)
{
	(void)any_symbol;
	// Inverted empty set.
	// TODO - process "AnySymbol" with exclusions.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::SpecificSymbol& specific_symbol)
{
	return OneOf{ {specific_symbol.code}, {}, false };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::String& string)
{
	const auto str_utf32= Utf8ToUtf32(string.str);
	if(!str_utf32.empty())
		return OneOf{ {str_utf32.front()}, {}, false };
	return GetPossibleStartSybmols(string.next);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::OneOf& one_of)
{
	return OneOf{ one_of.variants, one_of.ranges, one_of.inverse_flag };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::Alternatives& alternatives)
{
	OneOf res;
	for(const GraphElements::NodePtr& next : alternatives.next)
		res= CombineSymbolSets(res, GetPossibleStartSybmols(next));

	return res;
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::GroupStart& group_start)
{
	return GetPossibleStartSybmols(group_start.next);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::GroupEnd& group_end)
{
	return GetPossibleStartSybmols(group_end.next);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::BackReference& back_reference)
{
	(void)back_reference;
	// Any symbol is possible in backreference.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::LookAhead& look_ahead)
{
	(void)look_ahead;

	// Any symbol is possible in look_ahead.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::LookBehind& look_behind)
{
	(void)look_behind;

	// Any symbol is possible in look_behind.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::StringStartAssertion& string_start_assertion)
{
	(void)string_start_assertion;

	// Fail possessification optimization in such case - return all possible symbols.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::StringEndAssertion& string_end_assertion)
{
	(void)string_end_assertion;

	// Fail possessification optimization in such case - return all possible symbols.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::ConditionalElement& conditional_element)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(conditional_element.next_true),
		GetPossibleStartSybmols(conditional_element.next_false));
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounterReset& sequence_counter_reset)
{
	return GetPossibleStartSybmols(sequence_counter_reset.next);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::SequenceCounter& sequence_counter)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(sequence_counter.next_iteration),
		GetPossibleStartSybmols(sequence_counter.next_sequence_end));
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::NextWeakNode& next_weak)
{
	return GetPossibleStartSybmols(next_weak.next.lock());
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::PossessiveSequence& possessive_sequence)
{
	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(possessive_sequence.sequence_element),
		GetPossibleStartSybmols(possessive_sequence.next));
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence)
{
	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(fixed_length_element_sequence.sequence_element),
		GetPossibleStartSybmols(fixed_length_element_sequence.next));
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::AtomicGroup& atomic_group)
{
	// Combine for cases of empty atomic group. TODO - is this really needed? Or maybe symbols set will contain all symbols in empty group?
	return CombineSymbolSets(
		GetPossibleStartSybmols(atomic_group.next),
		GetPossibleStartSybmols(atomic_group.group_element));
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::SubroutineEnter& subroutine_enter)
{
	return GetPossibleStartSybmols(subroutine_enter.subroutine_node);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::SubroutineLeave& subroutine_leave)
{
	(void)subroutine_leave;

	// Any symbol is possible after subroutine leave.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::StateSave& state_save)
{
	return GetPossibleStartSybmols(state_save.next);
}

OneOf GetPossibleStartSybmolsImpl(const GraphElements::StateRestore& state_restore)
{
	return GetPossibleStartSybmols(state_restore.next);
}

using NodeEnumerationFunction= std::function<void(const GraphElements::NodePtr&)>;
using VisitedNodesSet= std::unordered_set<GraphElements::NodePtr>;

void EnumerateAllNodesOnceImpl(
	const NodeEnumerationFunction& func,
	VisitedNodesSet& visited_nodes_set,
	const GraphElements::NodePtr& node);

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::AnySymbol& any_symbol)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, any_symbol.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SpecificSymbol& specific_symbol)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, specific_symbol.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::String& string)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, string.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::OneOf& one_of)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, one_of.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::Alternatives& alternatives)
{
	for(const auto& next : alternatives.next)
		EnumerateAllNodesOnceImpl(func, visited_nodes_set, next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::GroupStart& group_start)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, group_start.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::GroupEnd& group_end)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, group_end.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::BackReference& back_reference)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, back_reference.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::LookAhead& look_ahead)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, look_ahead.look_graph);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, look_ahead.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::LookBehind& look_behind)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, look_behind.look_graph);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, look_behind.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::StringStartAssertion& string_start_assertion)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, string_start_assertion.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::StringEndAssertion& string_end_assertion)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, string_end_assertion.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::ConditionalElement& conditional_element)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, conditional_element.condition_node);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, conditional_element.next_true);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, conditional_element.next_false);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SequenceCounterReset& sequence_counter_reset)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, sequence_counter_reset.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SequenceCounter& sequence_counter)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, sequence_counter.next_iteration);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, sequence_counter.next_sequence_end);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::NextWeakNode& next_weak_node)
{
	if(const auto next= next_weak_node.next.lock())
		EnumerateAllNodesOnceImpl(func, visited_nodes_set, next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::PossessiveSequence& possesive_sequence)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, possesive_sequence.sequence_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, possesive_sequence.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, fixed_length_element_sequence.sequence_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, fixed_length_element_sequence.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::AtomicGroup& atomic_group)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, atomic_group.group_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, atomic_group.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SubroutineEnter& subroutine_enter)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, subroutine_enter.subroutine_node);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, subroutine_enter.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SubroutineLeave& subroutine_leave)
{
	(void)func;
	(void)visited_nodes_set;
	(void)subroutine_leave;
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::StateSave& state_save)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, state_save.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::StateRestore& state_restore)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, state_restore.next);
}

void EnumerateAllNodesOnceImpl(
	const NodeEnumerationFunction& func,
	VisitedNodesSet& visited_nodes_set,
	const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return;

	if(visited_nodes_set.count(node) != 0)
		return;
	visited_nodes_set.insert(node);

	func(node);

	return std::visit([&](const auto& el){ return EnumerateAllNodesOnceVisitImpl(func, visited_nodes_set, el); }, *node);
}

void EnumerateAllNodesOnce(const NodeEnumerationFunction& func, const GraphElements::NodePtr& start_node)
{
	VisitedNodesSet nodes_set;
	return EnumerateAllNodesOnceImpl(func, nodes_set, start_node);
}

void ApplyAlternativesBacktrackingEliminationOptimizationToNode(const GraphElements::NodePtr& node)
{
	/* Perform following optimization:

	If alternative node has only two branches and both branches starts with some fixed symbols
	and first alternative branch is single symobl/string/one_of
	it is possible to disable backtracking after matching of first element of first alternative,
	since if it matches, the second alternative is guaranteed not to match.
	 */

	const auto alternatives= std::get_if<GraphElements::Alternatives>(&*node);
	if(alternatives == nullptr)
		return;

	if(alternatives->next.size() != 2)
		return;
	const GraphElements::NodePtr first_alternative= alternatives->next[0];
	const GraphElements::NodePtr second_alternative= alternatives->next[1];

	const OneOf start_symbols_first= GetPossibleStartSybmols(first_alternative);
	const OneOf start_symbols_second= GetPossibleStartSybmols(second_alternative);
	if(HasIntersection(start_symbols_first, start_symbols_second))
		return;

	GraphElements::NodePtr* next;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(&*first_alternative))
		next= &specific_symbol->next;
	else if(const auto string= std::get_if<GraphElements::String>(&*first_alternative))
		next= &string->next;
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(&*first_alternative))
		next= &one_of->next;
	else
		return; // Unsupported kind.
}

void ApplyAlternativesBacktrackingEliminationOptimization(const GraphElements::NodePtr& graph_start)
{
	EnumerateAllNodesOnce(
		ApplyAlternativesBacktrackingEliminationOptimizationToNode,
		graph_start);
}

} // namespace

OneOf GetPossibleStartSybmols(const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return OneOf{}; // Empty set of symbols.

	return std::visit([&](const auto& el){ return GetPossibleStartSybmolsImpl(el); }, *node);
}

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

RegexGraphBuildResult OptimizeRegexGraph(RegexGraphBuildResult input_graph)
{
	RegexGraphBuildResult result= std::move(input_graph);

	ApplyAlternativesBacktrackingEliminationOptimization(result.root);

	return result;
}

} // namespace RegPanzer
