#include "Matcher.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/ConvertUTF.h>
#include "PopLLVMWarnings.hpp"
#include <cassert>
#include <unordered_map>

namespace RegPanzer
{

namespace
{

struct State
{
	std::string_view str;
	std::string_view groups[10];
	llvm::DenseMap<GraphElements::SequenceId, size_t> sequence_counters;
	llvm::SmallVector<GraphElements::NodePtr, 8> subroutines_return_stack;

	struct SubroutineEnterSaveState
	{
		llvm::SmallVector<std::pair<GraphElements::SequenceId, size_t>, 6> sequence_counters;
		llvm::SmallVector<std::pair<size_t, std::string_view>, 4> groups;
		const SubroutineEnterSaveState* prev= nullptr;
	};

	const SubroutineEnterSaveState* saved_state= nullptr;
};

std::optional<CharType> ExtractCodePoint(State& state)
{
	std::string_view& str= state.str;

	llvm::UTF32 code= 0;

	const auto src_start_initial= reinterpret_cast<const llvm::UTF8*>(str.data());
	auto src_start= src_start_initial;
	auto target_start= &code;

	const auto res= llvm::ConvertUTF8toUTF32(&src_start, src_start + str.size(), &target_start, target_start + 1, llvm::strictConversion);

	if(target_start != &code + 1 || !(res == llvm::conversionOK || res == llvm::targetExhausted))
		return std::nullopt;

	str.remove_prefix(size_t(src_start - src_start_initial));
	return CharType(code);
}

bool MatchNode(const GraphElements::NodePtr& node, State& state);

bool MatchNodeImpl(const GraphElements::AnySymbol& node, State& state)
{
	return ExtractCodePoint(state) && MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::SpecificSymbol& node, State& state)
{
	return ExtractCodePoint(state) == node.code && MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::OneOf& node, State& state)
{
	const auto code= ExtractCodePoint(state);
	if(code == std::nullopt)
		return false;

	bool found= false;
	for(const CharType& v : node.variants)
		if(*code == v)
			found= true;

	for(const auto& range : node.ranges)
		if(*code >= range.first && *code <= range.second)
			found= true;

	return (found ^ node.inverse_flag) && MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::Alternatives& node, State& state)
{
	for(const GraphElements::NodePtr& alternative : node.next)
	{
		State state_copy= state;
		if(MatchNode(alternative, state_copy))
		{
			state= state_copy;
			return true;
		}
	}

	return false;
}

bool MatchNodeImpl(const GraphElements::GroupStart& node, State& state)
{
	if(node.index >= 1 && node.index <= 9)
		state.groups[node.index]= state.str.substr(0, 0);

	return MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::GroupEnd& node, State& state)
{
	if(node.index >= 1 && node.index <= 9)
	{
		const char* const ptr= state.groups[node.index].data();
		const auto size= size_t(state.str.data() - ptr);
		state.groups[node.index]= std::string_view(ptr, size);
	}

	return MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::BackReference& node, State& state)
{
	if(node.index >= 1 && node.index <= 9)
	{
		const std::string_view prev_value= state.groups[node.index];
		if(state.str.size() >= prev_value.size() && state.str.substr(0, prev_value.size()) == prev_value)
		{
			state.str.remove_prefix(prev_value.size());
			return MatchNode(node.next, state);
		}
	}

	return false;
}

bool MatchNodeImpl(const GraphElements::Look& node, State& state)
{
	if(node.forward)
	{
		State state_copy= state;
		return (!node.positive ^ MatchNode(node.look_graph, state_copy)) && MatchNode(node.next, state);
	}
	else
	{
		assert(false && "not implemented yet!");
		return false;
	}
}

bool MatchNodeImpl(const GraphElements::ConditionalElement& node, State& state)
{
	State state_copy= state;
	if(MatchNode(node.condition_node, state_copy))
		return MatchNode(node.next_true, state);
	else
		return MatchNode(node.next_false, state);
}

bool MatchNodeImpl(const GraphElements::SequenceCounterReset& node, State& state)
{
	state.sequence_counters[node.id]= 0;
	return MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::SequenceCounter& node, State& state)
{
	const size_t sequence_counter= state.sequence_counters[node.id];
	++state.sequence_counters[node.id];

	if(sequence_counter < node.min_elements)
		return MatchNode(node.next_iteration, state);
	else if(sequence_counter >= node.max_elements)
		return MatchNode(node.next_sequence_end, state);
	else
	{
		State state_copy= state;
		if(node.greedy)
		{
			if(MatchNode(node.next_iteration, state_copy))
			{
				state= state_copy;
				return true;
			}
			else
				return MatchNode(node.next_sequence_end, state);
		}
		else
		{
			if(MatchNode(node.next_sequence_end, state_copy))
			{
				state= state_copy;
				return true;
			}
			else
				return MatchNode(node.next_iteration, state);
		}
	}
}

bool MatchNodeImpl(const GraphElements::NextWeakNode& node, State& state)
{
	const auto next= node.next.lock();
	assert(next != nullptr);
	return MatchNode(next, state);
}

bool MatchNodeImpl(const GraphElements::PossessiveSequence& node, State& state)
{
	for(size_t i= 0; i < node.max_elements; ++i)
	{
		State state_copy= state;
		if(!MatchNode(node.sequence_element, state_copy))
		{
			if(i < node.min_elements)
				return false;
			break;
		}
		state= state_copy;
	}

	return MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::AtomicGroup& node, State& state)
{
	if(MatchNode(node.group_element, state))
		return MatchNode(node.next, state);

	return false;
}

bool MatchNodeImpl(const GraphElements::SubroutineEnter& node, State& state)
{
	state.subroutines_return_stack.push_back(node.next);
	return MatchNode(node.subroutine_node, state);
}

bool MatchNodeImpl(const GraphElements::SubroutineLeave&, State& state)
{
	if(state.subroutines_return_stack.empty())
		return MatchNode(nullptr, state);

	const auto next_node= state.subroutines_return_stack.back();
	state.subroutines_return_stack.pop_back();
	return MatchNode(next_node, state);
}

bool MatchNodeImpl(const GraphElements::StateSave& node, State& state)
{
	State::SubroutineEnterSaveState state_to_save;

	state_to_save.sequence_counters.reserve(node.sequence_counters_to_save.size());
	for(const GraphElements::SequenceId sequence_id : node.sequence_counters_to_save)
	{
		state_to_save.sequence_counters.emplace_back(sequence_id, state.sequence_counters[sequence_id]);
		state.sequence_counters[sequence_id]= 0; // TODO - do we need to zero it here?
	}

	state_to_save.groups.reserve(node.groups_to_save.size());
	for(const size_t group_id : node.groups_to_save)
	{
		state_to_save.groups.emplace_back(group_id, state.groups[group_id]);
		state.groups[group_id] = std::string_view(); // TODO - do we need to reset it here?
	}

	state_to_save.prev= state.saved_state;
	state.saved_state= &state_to_save;

	return MatchNode(node.next, state);
}

bool MatchNodeImpl(const GraphElements::StateRestore& node, State& state)
{
	assert(state.saved_state != nullptr);

	for(const auto& sequence_counter_pair : state.saved_state->sequence_counters)
		state.sequence_counters[sequence_counter_pair.first]= sequence_counter_pair.second;

	for(const auto& group_pair : state.saved_state->groups)
		state.groups[group_pair.first]= group_pair.second;

	state.saved_state= state.saved_state->prev;

	return MatchNode(node.next, state);
}

bool MatchNode(const GraphElements::NodePtr& node, State& state)
{
	if(node == nullptr)
		return true;

	return std::visit([&](const auto& el){ return MatchNodeImpl(el, state); }, *node);
}

} // namespace

MatchResult Match(const GraphElements::NodePtr& node, std::string_view str)
{
	for(size_t i= 0; i < str.size(); ++i)
	{
		State state;
		state.str= str.substr(i, str.size() - i);
		if(MatchNode(node, state))
			return str.substr(i, str.size() - state.str.size() - i);
	}

	return std::nullopt;
}

} // namespace RegPanzer
