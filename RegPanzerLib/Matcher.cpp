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

std::optional<CharType> ExtractCodePoint(MatchInput& str)
{
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

bool MatchChain(const RegexElementsChain& chain, MatchInput& str);

bool MatchElementImpl(const AnySymbol&, MatchInput& str)
{
	return ExtractCodePoint(str) != std::nullopt;
}

bool MatchElementImpl(const SpecificSymbol& specific_symbol, MatchInput& str)
{
	return ExtractCodePoint(str) == specific_symbol.code;
}

bool MatchElementImpl(const OneOf& one_of, MatchInput& str)
{
	if(const auto code= ExtractCodePoint(str))
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

bool MatchElementImpl(const Group& group, MatchInput& str)
{
	return MatchChain(group.elements, str);
}

bool MatchElementImpl(const BackReference& back_reference, MatchInput& str)
{
	// TODO
	(void)back_reference;
	(void)str;
	return false;
}

bool MatchElementImpl(const Alternatives& alternatives, MatchInput& str)
{
	for(const RegexElementsChain& chain : alternatives.alternatives)
	{
		MatchInput range_copy= str;
		if(MatchChain(chain, range_copy))
		{
			str= range_copy;
			return true;
		}
	}

	return false;
}

bool MatchElementImpl(const Look& look, const MatchInput str)
{
	if(look.forward)
	{
		MatchInput range_copy= str;
		return (!look.positive) ^ MatchChain(look.elements, range_copy);
	}
	else
	{
		// TODO
		assert(false && "not implemented yet!");
		return false;
	}
}


bool MatchElement(const RegexElementFull::ElementType& element, MatchInput& str)
{
	return std::visit([&](const auto& el){ return MatchElementImpl(el, str); }, element);
}

bool MatchElementFull(const RegexIterator begin, const RegexIterator end, MatchInput& str)
{
	if(begin == end)
		return true;
	const RegexElementFull& element= *begin;

	switch(element.seq.mode)
	{
	case SequenceMode::Greedy:
		{
			std::optional<MatchInput> last_success_pos;

			for(size_t i= 0; ; ++i)
			{
				// Check tail only after reaching minimum number of elements.
				if(i >= element.seq.min_elements)
				{
					MatchInput range_copy= str;
					if(MatchElementFull(std::next(begin), end, range_copy))
						last_success_pos= range_copy;
				}

				if(i == element.seq.max_elements || // Finish loop after tail check but before sequence element check.
					!MatchElement(element.el, str))
					break;
			}

			if(last_success_pos == std::nullopt)
				return false;

			str= *last_success_pos;
			return true;
		}

	case SequenceMode::Lazy:
		{
			for(size_t i= 0; ; ++i)
			{
				// Check tail only after reaching minimum number of elements.
				if(i >= element.seq.min_elements)
				{
					MatchInput range_copy= str;
					if(MatchElementFull(std::next(begin), end, range_copy))
					{
						// In lazy mode return on first tail match.
						str= range_copy;
						return true;
					}
				}

				if(i == element.seq.max_elements) // Finish loop after tail check but before sequence element check.
					break;

				MatchInput range_copy= str;
				if(!MatchElement(element.el, range_copy))
					break;
				str= range_copy;
			}

			return false;
		}

	case SequenceMode::Possessive:
		{
			// In possessive mode check only sequence elements itself and check tail later.
			for(size_t i= 0; i < element.seq.max_elements; ++i)
			{
				MatchInput range_copy= str;
				if(!MatchElement(element.el, range_copy))
				{
					if(i < element.seq.min_elements)
						return false;
					else
						break;
				}
				str= range_copy;
			}

			// Than match tail.
			return MatchElementFull(std::next(begin), end, str);
		}
	}

	assert(false && "Unreachable code!");
	return false;
}

bool MatchChain(const RegexElementsChain& chain, MatchInput& str)
{
	return MatchElementFull(chain.begin(), chain.end(), str);
}

} // namespace

MatchResult Match(const RegexElementsChain& regex, const std::string_view str)
{
	for(size_t i= 0; i < str.size(); ++i)
	{
		MatchInput range= str.substr(i, str.size() - i);
		if(MatchChain(regex, range))
			return str.substr(i, str.size() - range.size() - i);
	}

	return std::nullopt;
}

} // namespace RegPanzer
