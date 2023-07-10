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

std::optional<size_t> GetOneOfLength(const GraphElements::OneOf& one_of)
{
	size_t min_size= 100, max_size= 0;
	for(const CharType c : one_of.variants)
	{
		const CharType str_utf32[]{c, 0};
		const size_t size= Utf32ToUtf8(str_utf32).size();
		min_size= std::min(min_size, size);
		max_size= std::max(max_size, size);
	}

	for(const auto& range : one_of.ranges)
	{
		const CharType begin_str_utf32[]{range.first , 0};
		const CharType   end_str_utf32[]{range.second, 0};
		min_size= std::min(min_size, Utf32ToUtf8(begin_str_utf32).size());
		max_size= std::max(max_size, Utf32ToUtf8(  end_str_utf32).size());
	}

	if(min_size != max_size)
		return std::nullopt;

	return min_size;
}

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

OneOf GetPossibleStartSybmols(VisitedNodesSet& visited_nodes, GraphElements::NodePtr node);

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
	for(const GraphElements::NodePtr next : alternatives.next)
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


OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::PossessiveSequence& possessive_sequence)
{
	if(possessive_sequence.min_elements > 0)
		return GetPossibleStartSybmols(visited_nodes, possessive_sequence.sequence_element); // At least one iteration - can use only sequence element.

	// Combine for cases of empty sequence.
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, possessive_sequence.sequence_element),
		GetPossibleStartSybmols(visited_nodes, possessive_sequence.next));
}

OneOf GetPossibleStartSybmolsImpl(VisitedNodesSet& visited_nodes, const GraphElements::SingleRollbackPointSequence& single_rollback_point_sequence)
{
	return CombineSymbolSets(
		GetPossibleStartSybmols(visited_nodes, single_rollback_point_sequence.sequence_element),
		GetPossibleStartSybmols(visited_nodes, single_rollback_point_sequence.next));
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

OneOf GetPossibleStartSybmols(VisitedNodesSet& visited_nodes, const GraphElements::NodePtr node)
{
	if(node == nullptr)
		return OneOf{}; // Empty set of symbols.

	if(visited_nodes.count(node) != 0)
		return OneOf{}; // Already visited this node. This is possible for sequences with zero possible element size.
	visited_nodes.insert(node);

	return std::visit([&](const auto& el){ return GetPossibleStartSybmolsImpl(visited_nodes, el); }, *node);
}

OneOf GetPossibleStartSybmolsEntry(const GraphElements::NodePtr node)
{
	VisitedNodesSet visited_nodes;
	return GetPossibleStartSybmols(visited_nodes, node);
}

//
// Node enumeration
//

using NodeEnumerationFunction= std::function<void(GraphElements::NodePtr)>;

void EnumerateAllNodesOnceImpl(
	const NodeEnumerationFunction& func,
	VisitedNodesSet& visited_nodes_set,
	GraphElements::NodePtr node);

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

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::PossessiveSequence& possesive_sequence)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, possesive_sequence.sequence_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, possesive_sequence.next);
}

void EnumerateAllNodesOnceVisitImpl(const NodeEnumerationFunction& func, VisitedNodesSet& visited_nodes_set, const GraphElements::SingleRollbackPointSequence& single_rollback_point_sequence)
{
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, single_rollback_point_sequence.sequence_element);
	EnumerateAllNodesOnceImpl(func, visited_nodes_set, single_rollback_point_sequence.next);
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

void EnumerateAllNodesOnce(const NodeEnumerationFunction& func, const GraphElements::NodePtr start_node)
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
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(node))
	{
		if(const auto specific_symbol_tail= std::get_if<GraphElements::SpecificSymbol>(specific_symbol->next))
		{
			// Append symbol to symbol.
			GraphElements::String string;
			const CharType str_utf32[]{specific_symbol->code, specific_symbol_tail->code, 0};
			string.str= Utf32ToUtf8(str_utf32);
			string.next= specific_symbol_tail->next;

			*node= std::move(string);
			return true;
		}
		if(const auto string_tail= std::get_if<GraphElements::String>(specific_symbol->next))
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
	if(const auto string= std::get_if<GraphElements::String>(node))
	{
		if(const auto specific_symbol_tail= std::get_if<GraphElements::SpecificSymbol>(string->next))
		{
			// Append string to symbol.
			const CharType str_utf32[]{specific_symbol_tail->code, 0};
			string->str+= Utf32ToUtf8(str_utf32);

			GraphElements::NodePtr next= specific_symbol_tail->next;
			string->next= std::move(next);

			return true;
		}
		if(const auto string_tail= std::get_if<GraphElements::String>(string->next))
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

void ApplySymbolsCombiningOptimization(const GraphElements::NodePtr graph_start)
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
// AlternativeStartunite
//

std::string GetNodeStartString(const GraphElements::NodePtr node)
{
	if(const auto string= std::get_if<GraphElements::String>(node))
		return string->str;

	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(node))
	{
		const CharType str_utf32[]{specific_symbol->code, 0};
		return Utf32ToUtf8(str_utf32);
	}

	return "";
}

// Return none if can't cut. May return nullptr.
std::optional<GraphElements::NodePtr> CutNodeStartString(const GraphElements::NodePtr node, const size_t symbols_cut, GraphElements::NodesStorage& nodes_storage)
{
	if(const auto string= std::get_if<GraphElements::String>(node))
	{
		assert(symbols_cut <= string->str.size());

		if(symbols_cut == string->str.size())
			return string->next;

		GraphElements::String copy;
		copy.next= string->next;
		copy.str= string->str.substr(symbols_cut);
		return nodes_storage.Allocate(std::move(copy));
	}

	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(node))
	{
		const CharType str_utf32[]{specific_symbol->code, 0};
		std::string s= Utf32ToUtf8(str_utf32);
		assert(symbols_cut <= s.size());

		if(symbols_cut == s.size())
			return specific_symbol->next;

		GraphElements::String copy;
		copy.next= specific_symbol->next;
		copy.str= s.substr(symbols_cut);
		return nodes_storage.Allocate(std::move(copy));
	}

	return std::nullopt;
}

//
// Alternatives start unite.
//

bool ApplyAlternativeStartUniteToNode(const GraphElements::NodePtr node, GraphElements::NodesStorage& nodes_storage)
{
	/*
		If alternative variants starts with common prefix - extract it - move branching point further.
		This may be important for later optimizations.
	*/

	const auto alternatives= std::get_if<GraphElements::Alternatives>(node);
	if(alternatives == nullptr || alternatives->next.empty())
		return false;

	std::vector<std::string> start_strings;
	start_strings.reserve(alternatives->next.size());
	for(const GraphElements::NodePtr branch : alternatives->next)
		start_strings.push_back(GetNodeStartString(branch));

	size_t num_common_symbols= 0;
	for(size_t i= 0; ; ++i)
	{
		bool matches= true;

		if(start_strings[0].size() <= i)
			break;

		for(size_t j= 1; j < start_strings.size(); ++j)
		{
			if(start_strings[j].size() <= i || start_strings[j][i] != start_strings[0][i])
			{
				matches= false;
				break;
			}
		}

		if(!matches)
			break;

		++num_common_symbols;
	}

	if(num_common_symbols == 0)
		return false; // No common prefix.

	// Cut prefix from next nodes.
	std::vector<GraphElements::NodePtr> next_cut;
	next_cut.reserve(alternatives->next.size());
	for(const GraphElements::NodePtr branch : alternatives->next)
	{
		const std::optional<GraphElements::NodePtr> cut= CutNodeStartString(branch, num_common_symbols, nodes_storage);
		if(cut == std::nullopt)
			return false;

		next_cut.push_back(*cut);
	}

	// Create new alternatives node.
	const GraphElements::NodePtr new_alternatives= nodes_storage.Allocate(GraphElements::Alternatives{std::move(next_cut)});

	// Replace this node with new alternatives.
	GraphElements::String string_node;
	string_node.str= start_strings[0].substr(0, num_common_symbols);
	string_node.next= new_alternatives;
	*node= std::move(string_node);

	return true;
}

void ApplyAlternativeStartUnite(const GraphElements::NodePtr graph_start, GraphElements::NodesStorage& nodes_storage)
{
	// Perform several steps to ensure full combination.
	while(true)
	{
		bool something_changed= false;
		EnumerateAllNodesOnce(
			[&](const GraphElements::NodePtr node)
			{
				if(ApplyAlternativeStartUniteToNode(node, nodes_storage))
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

void ApplyAlternativesPossessificationOptimizationToNode(const GraphElements::NodePtr node, GraphElements::NodesStorage& nodes_storage)
{
	/*
		Perform following optimization:
		If alternative node has only two branches and both branches starts with some fixed symbols
		and first alternative branch is single symobl/string/one_of
		it is possible to disable backtracking after matching of first element of first alternative,
		since if it matches, the second alternative is guaranteed not to match.
	*/

	const auto alternatives= std::get_if<GraphElements::Alternatives>(node);
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
	GraphElements::NodePtr next= nullptr;
	GraphElements::NodePtr first_alternative_modified= nullptr;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(first_alternative))
	{
		next= specific_symbol->next;
		GraphElements::SpecificSymbol copy= *specific_symbol;
		copy.next= nullptr;
		first_alternative_modified= nodes_storage.Allocate(std::move(copy));
	}
	else if(const auto string= std::get_if<GraphElements::String>(first_alternative))
	{
		next= string->next;
		GraphElements::String copy= *string;
		copy.next= nullptr;
		first_alternative_modified= nodes_storage.Allocate(std::move(copy));
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(first_alternative))
	{
		next= one_of->next;
		GraphElements::OneOf copy= *one_of;
		copy.next= nullptr;
		first_alternative_modified= nodes_storage.Allocate(std::move(copy));
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

void ApplyAlternativesPossessificationOptimization(const GraphElements::NodePtr graph_start, GraphElements::NodesStorage& nodes_storage)
{
	EnumerateAllNodesOnce(
		[&](const GraphElements::NodePtr node)
		{
			ApplyAlternativesPossessificationOptimizationToNode(node, nodes_storage);
		},
		graph_start);
}

void ApplyFixedLengthElementSequenceOptimizationForNode(const GraphElements::NodePtr node,  GraphElements::NodesStorage& nodes_storage)
{
	// For now apply the optimization only for sequences, implemented via alternatives node.
	const auto alternatives= std::get_if<GraphElements::Alternatives>(node);
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
	GraphElements::NodePtr sequence_element= nullptr;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(first_alternative))
	{
		if(specific_symbol->next != node)
			return; // Too complicated sequence body.

		const CharType str_utf32[]{specific_symbol->code, 0};
		element_length= Utf32ToUtf8(str_utf32).size();

		GraphElements::SpecificSymbol copy= *specific_symbol;
		copy.next= nullptr;
		sequence_element= nodes_storage.Allocate(std::move(copy));

	}
	else if(const auto string= std::get_if<GraphElements::String>(first_alternative))
	{
		if(string->next != node)
			return; // Too complicated sequence body.

		element_length= string->str.size();

		GraphElements::String copy= *string;
		copy.next= nullptr;
		sequence_element= nodes_storage.Allocate(std::move(copy));
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(first_alternative))
	{
		if(one_of->next != node)
			return; // Too complicated sequence body.

		const auto on_of_length= GetOneOfLength(*one_of);
		if(on_of_length == std::nullopt)
			return;

		element_length= *on_of_length;

		GraphElements::OneOf copy= *one_of;
		copy.next= nullptr;
		sequence_element= nodes_storage.Allocate(std::move(copy));
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

void ApplyFixedLengthElementSequenceOptimization(const GraphElements::NodePtr graph_start, GraphElements::NodesStorage& nodes_storage)
{
	EnumerateAllNodesOnce(
		[&](const GraphElements::NodePtr node)
		{
			ApplyFixedLengthElementSequenceOptimizationForNode(node, nodes_storage);
		},
		graph_start);
}

//
// Sequence with single rollback point ooptimiz<ation.
//

void ApplySequenceWithSingleRollbackPointOptimizationToNode(const GraphElements::NodePtr node, GraphElements::NodesStorage& nodes_storage)
{
	/*
		Use following optimization:
		if sequence element has fixed length and element after sequence has fixed small length and this is the end of whole regexp,
		create sequence with single rollback point.
		On each iteration of the sequence perform evaluation of both sequence element and element after sequence.
		If evaluation of element after the sequence was successfull - save state for this evaluation.
		Perform sequence element match until first fail. Than return last saved state for element after the sequence (if it is non-empty).
		It is necessary to have simple element after sequence (like single char or a small fixed string), because its matching is performing on each sequence iteration step.
	*/

	// For now apply the optimization only for sequences, implemented via alternatives node.
	const auto alternatives= std::get_if<GraphElements::Alternatives>(node);
	if(alternatives == nullptr)
		return;

	// Assume sequence is implemetded via alternatives node with loop trough first alternative path.
	// There is no reason to optimize sequences, implemented via second alternative path, because such sequences will be optimized
	// by the compiler backend, because they are tail calls.

	if(alternatives->next.size() != 2)
		return;
	const GraphElements::NodePtr first_alternative= alternatives->next[0];
	const GraphElements::NodePtr second_alternative= alternatives->next[1];

	// Support only single simple element after sequence.

	size_t element_after_alternative_length= 0;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(second_alternative))
	{
		if(specific_symbol->next != nullptr)
			return; // Too complicated tail.

		const CharType str_utf32[]{specific_symbol->code, 0};
		element_after_alternative_length= Utf32ToUtf8(str_utf32).size();
	}
	else if(const auto string= std::get_if<GraphElements::String>(second_alternative))
	{
		if(string->next != nullptr)
			return; // Too complicated tail.

		element_after_alternative_length= string->str.size();
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(second_alternative))
	{
		if(one_of->next != nullptr)
			return; // Too complicated tail.

		const auto on_of_length= GetOneOfLength(*one_of);
		if(on_of_length == std::nullopt)
			return;
		element_after_alternative_length= *on_of_length;
	}
	else
		return; // Unsupported kind.

	// For now support only sequence body, consisting of single simple element.
	size_t sequence_element_length= 0;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(first_alternative))
	{
		if(specific_symbol->next != node)
			return; // Too complicated sequence body.

		const CharType str_utf32[]{specific_symbol->code, 0};
		sequence_element_length= Utf32ToUtf8(str_utf32).size();
	}
	else if(const auto string= std::get_if<GraphElements::String>(first_alternative))
	{
		if(string->next != node)
			return; // Too complicated sequence body.

		sequence_element_length= string->str.size();
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(first_alternative))
	{
		if(one_of->next != node)
			return; // Too complicated sequence body.

		const auto on_of_length= GetOneOfLength(*one_of);
		if(on_of_length == std::nullopt)
			return;
		sequence_element_length= *on_of_length;
	}
	else
		return; // Unsupported kind.

	// Check if this optimization has sence.
	const bool element_after_alternative_length_is_moderate=
		element_after_alternative_length <= 2 ||
		element_after_alternative_length * 2 <= sequence_element_length;
	if(!element_after_alternative_length_is_moderate)
		return;

	// Create sequence element node.
	GraphElements::NodePtr sequence_element_node= nullptr;
	if(const auto specific_symbol= std::get_if<GraphElements::SpecificSymbol>(first_alternative))
	{
		GraphElements::SpecificSymbol copy= *specific_symbol;
		copy.next= nullptr;
		sequence_element_node= nodes_storage.Allocate(std::move(copy));

	}
	else if(const auto string= std::get_if<GraphElements::String>(first_alternative))
	{
		GraphElements::String copy= *string;
		copy.next= nullptr;
		sequence_element_node= nodes_storage.Allocate(std::move(copy));
	}
	else if(const auto one_of= std::get_if<GraphElements::OneOf>(first_alternative))
	{
		GraphElements::OneOf copy= *one_of;
		copy.next= nullptr;
		sequence_element_node= nodes_storage.Allocate(std::move(copy));
	}
	else
		return; // Unsupported kind.

	// Replace this alternatives node with optimized one.
	GraphElements::SingleRollbackPointSequence sequenece;
	sequenece.sequence_element= sequence_element_node;
	sequenece.next= second_alternative;

	*node= GraphElements::Node(std::move(sequenece));
}

void ApplySequenceWithSingleRollbackPointOptimization(const GraphElements::NodePtr graph_start, GraphElements::NodesStorage& nodes_storage)
{
	EnumerateAllNodesOnce(
		[&](const GraphElements::NodePtr node)
		{
			ApplySequenceWithSingleRollbackPointOptimizationToNode(node, nodes_storage);
		},
		graph_start);
}

} // namespace

RegexGraphBuildResult OptimizeRegexGraph(RegexGraphBuildResult input_graph)
{
	RegexGraphBuildResult result= std::move(input_graph);

	// Perform symbols combining and alternatives start unite.
	// Do this multiple times in order to re-combine strings, combined during alternatives start unite.
	for(size_t i= 0; i < 3; ++i)
	{
		ApplySymbolsCombiningOptimization(result.root);

		// Perform this step before performing alternatives possessification.
		ApplyAlternativeStartUnite(result.root, result.nodes_storage);
	}

	ApplyAlternativesPossessificationOptimization(result.root, result.nodes_storage);

	// Apply sequence with single rollback point optimization before fixed length sequence optimization, because it is faster.
	ApplySequenceWithSingleRollbackPointOptimization(result.root, result.nodes_storage);

	// Apply fixed length sequence optimization only after alternatives possessification optimization,
	// because first optimization is better (produces faster code).
	ApplyFixedLengthElementSequenceOptimization(result.root, result.nodes_storage);

	return result;
}

} // namespace RegPanzer
