#include "RegexpBuilder.hpp"

namespace RegPanzer
{

namespace
{

// Work in progress!
// TODO - write unit-tests.
std::optional<RegexpElementFull> ParseRegexpStringImpl(std::basic_string_view<CharType> str)
{
	if(str.empty())
		return std::nullopt;

	RegexpElementFull res;
	res.seq.min_elements= 1;
	res.seq.max_elements= 1;

	// Extract element.
	// TODO - porcess escape sequences.
	switch(str.front())
	{
	case '.':
		res.el= AnySymbol{};
		str.remove_prefix(1);
		break;

	case '[':
	{
		OneOf one_of;
		str.remove_prefix(1);
		while(!str.empty() && str.front() != ']')
		{
			// TODO - process escape sequences.

			const CharType c= str.front();
			str.remove_prefix(1);
			if(str.empty())
			{
				// TODO - handle error here
				return std::nullopt;
			}

			if(str.front() == '-')
			{
				str.remove_prefix(1);
				if(str.empty())
				{
					// TODO - handle error here
					return std::nullopt;
				}
				const CharType end_c= str.front();
				// TODO - validate range
				one_of.ranges.emplace_back(c, end_c);
				str.remove_prefix(1);
			}
			else
				one_of.variants.push_back(c);
		}

		if(str.empty() || str.front() != ']')
		{
			// TODO - handle error here
			return std::nullopt;
		}
		str.remove_prefix(1);

		res.el= std::move(one_of);
	}
		break;

	default:
		res.el= SpecificSymbol{ str.front() };
		str.remove_prefix(1);
		break;
	};

	if(str.empty())
		return res;

	// Try extract sequence.
	switch(str.front())
	{
	case '+':
		res.seq.min_elements= 1;
		res.seq.max_elements= std::numeric_limits<decltype(res.seq.max_elements)>::max();
		str.remove_prefix(1);
		break;

	case '*':
		res.seq.min_elements= 1;
		res.seq.max_elements= std::numeric_limits<decltype(res.seq.max_elements)>::max();
		str.remove_prefix(1);
		break;

	case '{':
		str.remove_prefix(1); // skip {

		res.seq.min_elements= 0;
		while(!str.empty() && str.front() >= '0' && str.front() <= '9')
		{
			res.seq.min_elements*= 10;
			res.seq.min_elements= str.front() - '0';
			str.remove_prefix(1);
		}

		if(str.empty())
		{
			// TODO - handle error here
			return std::nullopt;
		}
		if(str.front() == ',')
		{
			str.remove_prefix(1); // Skip ,

			res.seq.max_elements= 0;
			while(!str.empty() && str.front() >= '0' && str.front() <= '9')
			{
				res.seq.max_elements*= 10;
				res.seq.max_elements= str.front() - '0';
				str.remove_prefix(1);
			}
		}
		else
			res.seq.max_elements= res.seq.min_elements;

		if(str.empty() || str.front() != '}')
		{
			// TODO - handle error here
			return std::nullopt;
		}
		str.remove_prefix(1); // Skip }

		break;

	default:
		break;
	}

	if(!str.empty())
	{
		if(std::optional<RegexpElementFull> next= ParseRegexpStringImpl(str))
			res.next= std::make_unique<RegexpElementFull>(std::move(*next));
	}

	return res;
}

} // namespace

std::optional<RegexpElementFull> ParseRegexpString(const std::basic_string_view<CharType> str)
{
	return ParseRegexpStringImpl(str);
}

} // namespace RegPanzer
