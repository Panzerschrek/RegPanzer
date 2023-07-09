#include "../RegexGraphOptimizer.hpp"
#include "../Utils.hpp"

namespace RegPanzer
{

namespace
{

using VisitedNodesSet= std::unordered_set<GraphElements::NodePtr>;

//
// Start symbols stuff
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

OneOf GetAnySymbol()
{
	// Inverted empty set.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmols(VisitedNodesSet& visited_nodes, const GraphElements::NodePtr& node);

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::AnySymbol& any_symbol)
{
	(void)visited_nodes;
	(void)any_symbol;
	// Inverted empty set.
	// TODO - process "AnySymbol" with exclusions.
	return OneOf{ {}, {}, true };
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SpecificSymbol& specific_symbol)
{
	(void)visited_nodes;
	return OneOf{ {specific_symbol.code}, {}, false };
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::String& string)
{
	const auto str_utf32= Utf8ToUtf32(string.str);
	if(!str_utf32.empty())
		return OneOf{ {str_utf32.front()}, {}, false };
	return GetPossibleStartSybmols(visited_nodes, string.next);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::OneOf& one_of)
{
	(void)visited_nodes;
	return OneOf{ one_of.variants, one_of.ranges, one_of.inverse_flag };
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::Alternatives& alternatives)
{
	OneOf res;
	for(const GraphElements::NodePtr& next : alternatives.next)
		res= CombineSymbolSets(res, GetPossibleStartSybmols(visited_nodes, next));

	return res;
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::AlternativesPossessive& alternatives_possessive)
{
	return CombineSymbolSets(
			GetPossibleStartSybmols(visited_nodes, alternatives_possessive.path0_element),
			GetPossibleStartSybmols(visited_nodes, alternatives_possessive.path1_next));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::GroupStart& group_start)
{
	return GetPossibleStartSybmols(visited_nodes, group_start.next);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::GroupEnd& group_end)
{
	return GetPossibleStartSybmols(visited_nodes, group_end.next);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::BackReference& back_reference)
{
	(void)visited_nodes;
	(void)back_reference;
	// Any symbol is possible in backreference.
	 return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::LookAhead& look_ahead)
{
	(void)visited_nodes;
	(void)look_ahead;
	// Any symbol is possible in look_ahead.
	 return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::LookBehind& look_behind)
{
	(void)visited_nodes;
	(void)look_behind;
	// Any symbol is possible in look_behind.
	 return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::StringStartAssertion& string_start_assertion)
{
	(void)visited_nodes;
	(void)string_start_assertion;
	// Fail possessification optimization in such case - return all possible symbols.
	 return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::StringEndAssertion& string_end_assertion)
{
	(void)visited_nodes;
	(void)string_end_assertion;
	// Fail possessification optimization in such case - return all possible symbols.
	 return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::ConditionalElement& conditional_element)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, conditional_element.next_true),
		GetPossibleStartSybmols(visited_nodes, conditional_element.next_false));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SequenceCounterReset& sequence_counter_reset)
{
	return GetPossibleStartSybmols(visited_nodes, sequence_counter_reset.next);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SequenceCounter& sequence_counter)
{
	if(sequence_counter.min_elements > 0)
		return GetPossibleStartSybmols(visited_nodes, sequence_counter.next_iteration); // At least one iteration - can use only sequence element.

	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, sequence_counter.next_iteration),
		GetPossibleStartSybmols(visited_nodes, sequence_counter.next_sequence_end));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::NextWeakNode& next_weak)
{
	return GetPossibleStartSybmols(visited_nodes, next_weak.next.lock());
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::PossessiveSequence& possessive_sequence)
{
	if(possessive_sequence.min_elements > 0)
		return GetPossibleStartSybmols(visited_nodes, possessive_sequence.sequence_element); // At least one iteration - can use only sequence element.

	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, possessive_sequence.sequence_element),
		GetPossibleStartSybmols(visited_nodes, possessive_sequence.next));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::FixedLengthElementSequence& fixed_length_element_sequence)
{
	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, fixed_length_element_sequence.sequence_element),
		GetPossibleStartSybmols(visited_nodes, fixed_length_element_sequence.next));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::AtomicGroup& atomic_group)
{
	// Combine for cases of empty atomic group. TODO - is this really needed? Or maybe symbols set will contain all symbols in empty group?
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, atomic_group.next),
		GetPossibleStartSybmols(visited_nodes, atomic_group.group_element));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SubroutineEnter& subroutine_enter)
{
	return GetPossibleStartSybmols(visited_nodes, subroutine_enter.subroutine_node);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SubroutineLeave& subroutine_leave)
{
	(void)visited_nodes;
	(void)subroutine_leave;
	// Any symbol is possible after subroutine leave.
	return GetAnySymbol();
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::StateSave& state_save)
{
	return GetPossibleStartSybmols(visited_nodes, state_save.next);
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::StateRestore& state_restore)
{
	return GetPossibleStartSybmols(visited_nodes, state_restore.next);
}

OneOf GetPossibleStartSybmols(VisitedNodesSet& visited_nodes, const GraphElements::NodePtr& node)
{
	if(node == nullptr)
		return OneOf{}; // Empty set of symbols.

	if(visited_nodes.count(node) != 0)
		return OneOf{}; // Already visited this node. This is possible for sequences with zero possible element size.
	visited_nodes.insert(node);

	return std::visit([&](const auto& el){ return GetPossibleStartSybmolsImpl(visited_nodes, el); }, *node);
}

OneOf GetPossibleStartSybmolsEntry(const GraphElements::NodePtr& node)
{
	VisitedNodesSet visited_nodes;
	return GetPossibleStartSybmols(visited_nodes, node);
}

//
// Node enumeration
//

using NodeEnumerationFunction= std::function<void(const GraphElements::NodePtr)>;

void EnumerateAllNodesOnceImpl(
	const NodeEnumerationFunction& func,
	VisitedNodesSet& visited_nodes_set,
	const GraphElements::NodePtr node);

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

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::AlternativesPossessive& alternatives_possessive)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, alternatives_possessive.path0_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, alternatives_possessive.path0_next);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, alternatives_possessive.path1_next);
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
	const GraphElements::NodePtr node)
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

//
// Symbols combination.
//

// Returns true if something changed.
bool ApplySymbolsCombiningOptimizationToNode(const GraphElements::NodePtr node)
{
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(&*node))
	{
		if(const auto specific_symbol_tail= std::get_if<GraphElements::SpecificSymbol>(&*specific_symbol->next))
		{
			// Append symbol to symbol.
			GraphElements::String string;
			const CharType str_utf32[]{specific_symbol->code, specific_symbol_tail->code, 0};
			string.str= Utf32ToUtf8(str_utf32);
			string.next= specific_symbol_tail->next;

			*node= std::move(string);
			return true;
		}
		if(const auto string_tail= std::get_if<GraphElements::String>(&*specific_symbol->next))
		{
			// Append string to symbol.
			GraphElements::String string;
			const CharType str_utf32[]{specific_symbol->code, 0};
			string.str= Utf32ToUtf8(str_utf32);
			string.str+= string_tail->str;
			string.next= string_tail->next;

			*node= std::move(string);
			return true;
		}
	}
	if(const auto string= std::get_if<GraphElements::String>(&*node))
	{
		if(const auto specific_symbol_tail= std::get_if<GraphElements::SpecificSymbol>(&*string->next))
		{
			// Append string to symbol.
			const CharType str_utf32[]{specific_symbol_tail->code, 0};
			string->str+= Utf32ToUtf8(str_utf32);

			GraphElements::NodePtr next= specific_symbol_tail->next;
			string->next= std::move(next);

			return true;
		}
		if(const auto string_tail= std::get_if<GraphElements::String>(&*string->next))
		{
			// Append string to string.
			string->str+= string_tail->str;

			GraphElements::NodePtr next= string_tail->next;
			string->next= std::move(next);

			return true;
		}
	}

	return false;
}

void ApplySymbolsCombiningOptimization(const GraphElements::NodePtr& graph_start)
{
	// Perform several steps to ensure full combination.
	while(true)
	{
		bool something_changed= false;
		EnumerateAllNodesOnce(
			[&](const GraphElements::NodePtr node)
			{
				if(ApplySymbolsCombiningOptimizationToNode(node))
					something_changed= true;
			},
			graph_start);

		if(!something_changed)
			break;
	}
}

//
// Alternatives possessification.
//

void ApplyAlternativesPossessificationOptimizationToNode(const GraphElements::NodePtr node)
{
	/*
		Perform following optimization:
		If alternative node has only two branches and both branches starts with some fixed symbols
		and first alternative branch is single symobl/string/one_of
		it is possible to disable backtracking after matching of first element of first alternative,
		since if it matches, the second alternative is guaranteed not to match.
	*/

	const auto alternatives= std::get_if<GraphElements::Alternatives>(&*node);
	if(alternatives == nullptr)
		return;

	// TODO - support more than 2 branches - combine leftover branches into leftover "Alternative" node.
	if(alternatives->next.size() != 2)
		return;
	const GraphElements::NodePtr first_alternative= alternatives->next[0];
	const GraphElements::NodePtr second_alternative= alternatives->next[1];

	if(HasIntersection(GetPossibleStartSybmolsEntry(first_alternative), GetPossibleStartSybmolsEntry(second_alternative)))
		return;

	// Create copy of first alternative node in order to avoid modifying existing node.
	// TODO - create "weak_ptr" if needed?
	GraphElements::NodePtr next;
	GraphElements::NodePtr first_alternative_modified;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(&*first_alternative))
	{
		next= specific_symbol->next;
		GraphElements::SpecificSymbol copy= *specific_symbol;
		copy.next= nullptr;
		first_alternative_modified= std::make_shared<GraphElements::Node>(std::move(copy));
	}
	else if(const auto string= std::get_if<GraphElements::String>(&*first_alternative))
	{
		next= string->next;
		GraphElements::String copy= *string;
		copy.next= nullptr;
		first_alternative_modified= std::make_shared<GraphElements::Node>(std::move(copy));
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(&*first_alternative))
	{
		next= one_of->next;
		GraphElements::OneOf copy= *one_of;
		copy.next= nullptr;
		first_alternative_modified= std::make_shared<GraphElements::Node>(std::move(copy));
	}
	else
		return; // Unsupported kind.

	// Perform the optimization, replace node with new one.
	GraphElements::AlternativesPossessive alternatives_possessive;
	alternatives_possessive.path0_element= first_alternative_modified;
	alternatives_possessive.path0_next= next;
	alternatives_possessive.path1_next= second_alternative;

	*node= std::move(alternatives_possessive);
}

void ApplyAlternativesPossessificationOptimization(const GraphElements::NodePtr& graph_start)
{
	EnumerateAllNodesOnce(
		ApplyAlternativesPossessificationOptimizationToNode,
		graph_start);
}

void ApplyFixedLengthElementSequenceOptimizationForNode(const GraphElements::NodePtr node)
{
	// For now apply the optimization only for sequences, implemented via alternatives node.
	const auto alternatives= std::get_if<GraphElements::Alternatives>(&*node);
	if(alternatives == nullptr)
		return;

	// Assume sequence is implemetded via alternatives node with loop trough first alternative path.
	// There is no reason to optimize sequences, implemented via second alternative path, because such sequences will be optimized
	// by the compiler backend, because they are tail calls.

	if(alternatives->next.size() != 2)
		return;
	const GraphElements::NodePtr first_alternative= alternatives->next[0];
	const GraphElements::NodePtr second_alternative= alternatives->next[1];

	// For now support only sequence body, consisting of single simple element.
	size_t element_length= 0;
	GraphElements::NodePtr sequence_element;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(&*first_alternative))
	{
		if(specific_symbol->next != node)
			return; // Too complicated sequence body.

		const CharType str_utf32[]{specific_symbol->code, 0};
		element_length= Utf32ToUtf8(str_utf32).size();

		GraphElements::SpecificSymbol copy= *specific_symbol;
		copy.next= nullptr;
		sequence_element= std::make_shared<GraphElements::Node>(std::move(copy));

	}
	else if(const auto string= std::get_if<GraphElements::String>(&*first_alternative))
	{
		if(string->next != node)
			return; // Too complicated sequence body.

		element_length= string->str.size();

		GraphElements::String copy= *string;
		copy.next= nullptr;
		sequence_element= std::make_shared<GraphElements::Node>(std::move(copy));
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(&*first_alternative))
	{
		if(one_of->next != node)
			return; // Too complicated sequence body.

		size_t min_size= 100, max_size= 0;
		for(const CharType c : one_of->variants)
		{
			const CharType str_utf32[]{c, 0};
			const size_t size= Utf32ToUtf8(str_utf32).size();
			min_size= std::min(min_size, size);
			max_size= std::max(max_size, size);
		}

		for(const auto& range : one_of->ranges)
		{
			const CharType begin_str_utf32[]{range.first , 0};
			const CharType   end_str_utf32[]{range.second, 0};
			min_size= std::min(min_size, Utf32ToUtf8(begin_str_utf32).size());
			max_size= std::max(max_size, Utf32ToUtf8(  end_str_utf32).size());
		}

		if(min_size != max_size)
			return;

		element_length= max_size;

		GraphElements::OneOf copy= *one_of;
		copy.next= nullptr;
		sequence_element= std::make_shared<GraphElements::Node>(std::move(copy));
	}
	else
		return; // Unsupported kind.

	GraphElements::FixedLengthElementSequence fixed_length_element_sequence;
	fixed_length_element_sequence.next= second_alternative;
	fixed_length_element_sequence.sequence_element= std::move(sequence_element);
	fixed_length_element_sequence.min_elements= 0;
	fixed_length_element_sequence.max_elements= std::numeric_limits<size_t>::max();
	fixed_length_element_sequence.element_length= element_length;

	// Replace alternatives node with fixed length element sequence node.
	*node= GraphElements::Node(std::move(fixed_length_element_sequence));
}

void ApplyFixedLengthElementSequenceOptimization(const GraphElements::NodePtr& graph_start)
{
	EnumerateAllNodesOnce(
		ApplyFixedLengthElementSequenceOptimizationForNode,
		graph_start);
}

} // namespace

RegexGraphBuildResult OptimizeRegexGraph(RegexGraphBuildResult input_graph)
{
	RegexGraphBuildResult result= std::move(input_graph);

	ApplySymbolsCombiningOptimization(result.root);

	ApplyAlternativesPossessificationOptimization(result.root);

	// Apply fixed length sequence optimization only after alternatives possessification optimization,
	// because first optimization is better (produces faster code).
	ApplyFixedLengthElementSequenceOptimization(result.root);

	return result;
}

} // namespace RegPanzer
