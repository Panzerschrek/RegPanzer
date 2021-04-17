#include "Matcher.hpp"

namespace RegPanzer
{

namespace
{

using RegexpIterator = RegexpElementsChain::const_iterator;
using MatchInput = std::basic_string_view<CharType>;

bool MatchChain(const RegexpElementsChain& chain, MatchInput& str);

bool MatchElementImpl(const AnySymbol&, MatchInput& str)
{
	if(str.empty())
		return false;
	str.remove_prefix(1);
	return true;
}

bool MatchElementImpl(const SpecificSymbol& specific_symbol, MatchInput& str)
{
	if(str.empty() || str.front() != specific_symbol.code)
		return false;
	str.remove_prefix(1);
	return true;
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
	const CharType code= str.front();
	bool found= false;

	for(const CharType& v : one_of.variants)
	{
		if(code == v)
		{
			found= true;
			break;
		}
	}
	for(const auto& range : one_of.ranges)
	{
		if(code >= range.first && code <= range.second)
		{
			found= true;
			break;
		}
	}

	if(!found)
		return false;

	str.remove_prefix(1);
	return true;
}

bool MatchElement(const RegexpElementFull::ElementType& element, MatchInput& str)
{
	return std::visit([&](const auto& el){ return MatchElementImpl(el, str); }, element);
}

bool MatchElementFull(
	const RegexpElementFull& element,
	MatchInput& str,
	const RegexpIterator tail_begin,
	const RegexpIterator tail_end)
{
	if(element.seq.min_elements == 1 && element.seq.max_elements == 1)
	{
		MatchInput range_copy= str;

		const bool success=
			MatchElement(element.el, range_copy) &&
			(tail_begin == tail_end || MatchElementFull(*tail_begin, range_copy, std::next(tail_begin), tail_end));

		if(success)
			str= range_copy;
		return success;
	}
	else
	{
		std::optional<MatchInput> last_success_pos;

		{
			MatchInput range_copy= str;
			for(size_t i= 0; i <= element.seq.max_elements; ++i)
			{
				if(!MatchElement(element.el, range_copy))
				{
					if(i < element.seq.min_elements)
						return false;
					else
						break;
				}

				MatchInput range_for_tail= range_copy;
				const bool tail_success= tail_begin == tail_end || MatchElementFull(*tail_begin, range_for_tail, std::next(tail_begin), tail_end);
				if(tail_success)
					last_success_pos= range_for_tail;
			}
		}

		if(last_success_pos == std::nullopt)
			return false;

		str= *last_success_pos;
		return true;
	}
}

bool MatchChain(const RegexpElementsChain& chain, MatchInput& str)
{
	if(chain.empty())
		return true;

	return MatchElementFull(chain.front(), str, std::next(chain.begin()), chain.end());
}

} // namespace

MatchResult Match(const RegexpElementsChain& regexp, const std::basic_string_view<CharType> str)
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
