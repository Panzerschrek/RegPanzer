#include "Matcher.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/Support/ConvertUTF.h>
#include "PopLLVMWarnings.hpp"
#include <cassert>

namespace RegPanzer
{

namespace
{

using RegexIterator = RegexElementsChain::const_iterator;
using MatchInput = std::string_view;

constexpr size_t c_max_group= 9;

struct State
{
	MatchInput str;
	MatchInput groups[1 + c_max_group]; // Actually used 1-9
};

std::optional<CharType> ExtractCodePoint(State& state)
{
	MatchInput& str= state.str;

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

bool MatchChain(const RegexElementsChain& chain, State& state);

bool MatchElementImpl(const AnySymbol&, State& state)
{
	return ExtractCodePoint(state) != std::nullopt;
}

bool MatchElementImpl(const SpecificSymbol& specific_symbol, State& state)
{
	return ExtractCodePoint(state) == specific_symbol.code;
}

bool MatchElementImpl(const OneOf& one_of, State& state)
{
	if(const auto code= ExtractCodePoint(state))
	{
		for(const CharType& v : one_of.variants)
			if(*code == v)
				return !one_of.inverse_flag;

		for(const auto& range : one_of.ranges)
			if(*code >= range.first && *code <= range.second)
				return !one_of.inverse_flag;

		return one_of.inverse_flag;
	}

	return false;
}

bool MatchElementImpl(const Group& group, State& state)
{
	const char* const start= state.str.data();
	if(MatchChain(group.elements, state))
	{
		if(group.index >= 1 && group.index <= c_max_group)
		{
			const char* const end= state.str.data();
			state.groups[group.index]= MatchInput(start, size_t(end - start));
		}
		return true;
	}
	return false;
}

bool MatchElementImpl(const BackReference& back_reference, State& state)
{
	if(!(back_reference.index >= 0 && back_reference.index <= c_max_group))
		return false;

	const MatchInput prev_group= state.groups[back_reference.index];
	if(state.str.size() >= prev_group.size() && prev_group == state.str.substr(0, prev_group.size()))
	{
		state.str.remove_prefix(prev_group.size());
		return true;
	}

	return false;
}

bool MatchElementImpl(const Alternatives& alternatives, State& state)
{
	for(const RegexElementsChain& chain : alternatives.alternatives)
	{
		State state_copy= state;
		if(MatchChain(chain, state_copy))
		{
			state= state_copy;
			return true;
		}
	}

	return false;
}

bool MatchElementImpl(const Look& look, const State& state)
{
	if(look.forward)
	{
		State state_copy= state;
		return (!look.positive) ^ MatchChain(look.elements, state_copy);
	}
	else
	{
		// TODO
		assert(false && "not implemented yet!");
		return false;
	}
}

bool MatchElement(const RegexElementFull::ElementType& element, State& state)
{
	return std::visit([&](const auto& el){ return MatchElementImpl(el, state); }, element);
}

bool MatchElementFull(const RegexIterator begin, const RegexIterator end, State& state)
{
	if(begin == end)
		return true;
	const RegexElementFull& element= *begin;

	switch(element.seq.mode)
	{
	case SequenceMode::Greedy:
		{
			std::optional<State> last_success_state;

			for(size_t i= 0; ; ++i)
			{
				// Check tail only after reaching minimum number of elements.
				if(i >= element.seq.min_elements)
				{
					State state_copy= state;
					if(MatchElementFull(std::next(begin), end, state_copy))
						last_success_state= state_copy;
				}

				if(i == element.seq.max_elements || // Finish loop after tail check but before sequence element check.
					!MatchElement(element.el, state))
					break;
			}

			if(last_success_state == std::nullopt)
				return false;

			state= *last_success_state;
			return true;
		}

	case SequenceMode::Lazy:
		{
			for(size_t i= 0; ; ++i)
			{
				// Check tail only after reaching minimum number of elements.
				if(i >= element.seq.min_elements)
				{
					State state_copy= state;
					if(MatchElementFull(std::next(begin), end, state_copy))
					{
						// In lazy mode return on first tail match.
						state= state_copy;
						return true;
					}
				}

				if(i == element.seq.max_elements) // Finish loop after tail check but before sequence element check.
					break;

				State state_copy= state;
				if(!MatchElement(element.el, state_copy))
					break;
				state= state_copy;
			}

			return false;
		}

	case SequenceMode::Possessive:
		{
			// In possessive mode check only sequence elements itself and check tail later.
			for(size_t i= 0; i < element.seq.max_elements; ++i)
			{
				State state_copy= state;
				if(!MatchElement(element.el, state_copy))
				{
					if(i < element.seq.min_elements)
						return false;
					else
						break;
				}
				state= state_copy;
			}

			// Than match tail.
			return MatchElementFull(std::next(begin), end, state);
		}
	}

	assert(false && "Unreachable code!");
	return false;
}

bool MatchChain(const RegexElementsChain& chain, State& state)
{
	return MatchElementFull(chain.begin(), chain.end(), state);
}

} // namespace

MatchResult Match(const RegexElementsChain& regex, const std::string_view str)
{
	for(size_t i= 0; i < str.size(); ++i)
	{
		State state;
		state.str= str.substr(i, str.size() - i);
		if(MatchChain(regex, state))
			return str.substr(i, str.size() - state.str.size() - i);
	}

	return std::nullopt;
}

} // namespace RegPanzer
