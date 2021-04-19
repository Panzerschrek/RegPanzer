#include "Matcher.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/Support/ConvertUTF.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

using RegexpIterator = RegexpElementsChain::const_iterator;
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

bool MatchChain(const RegexpElementsChain& chain, MatchInput& str);

bool MatchElementImpl(const AnySymbol&, MatchInput& str)
{
	return ExtractCodePoint(str) != std::nullopt;
}

bool MatchElementImpl(const SpecificSymbol& specific_symbol, MatchInput& str)
{
	return ExtractCodePoint(str) == specific_symbol.code;
}

bool MatchElementImpl(const BracketExpression& bracket_expression, MatchInput& str)
{
	return MatchChain(bracket_expression.elements, str);
}

bool MatchElementImpl(const Alternatives& alternatives, MatchInput& str)
{
	for(const RegexpElementsChain& chain : alternatives.alternatives)
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

bool MatchElementImpl(const OneOf& one_of, MatchInput& str)
{
	if(str.empty())
		return false;

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

bool MatchElement(const RegexpElementFull::ElementType& element, MatchInput& str)
{
	return std::visit([&](const auto& el){ return MatchElementImpl(el, str); }, element);
}

bool MatchElementFull(const RegexpIterator begin, const RegexpIterator end, MatchInput& str)
{
	if(begin == end)
		return true;
	const RegexpElementFull& element= *begin;

	std::optional<MatchInput> last_success_pos;

	for(size_t i= 0; ; ++i)
	{
		// Check tail only after reqaching minimum number of elements.
		if(i >= element.seq.min_elements)
		{
			MatchInput range_copy= str;
			if(begin == end || MatchElementFull(std::next(begin), end, range_copy))
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

bool MatchChain(const RegexpElementsChain& chain, MatchInput& str)
{
	return MatchElementFull(chain.begin(), chain.end(), str);
}

} // namespace

MatchResult Match(const RegexpElementsChain& regexp, const std::string_view str)
{
	for(size_t i= 0; i < str.size(); ++i)
	{
		MatchInput range= str.substr(i, str.size() - i);
		if(MatchChain(regexp, range))
			return str.substr(i, str.size() - range.size() - i);
	}

	return std::nullopt;
}

} // namespace RegPanzer
