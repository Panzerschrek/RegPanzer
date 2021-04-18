#include "RegexpBuilder.hpp"
#include <llvm/Support/ConvertUTF.h>

namespace RegPanzer
{

namespace
{

// Work in progress!
std::optional<RegexpElementsChain> ParseRegexpStringImpl(std::basic_string_view<CharType>& str)
{
	RegexpElementsChain chain;

	while(!str.empty())
	{
		RegexpElementFull res;
		res.seq.min_elements= 1;
		res.seq.max_elements= 1;

		// Extract element.
		// TODO - porcess escape sequences.
		switch(str.front())
		{
		case '|':
		{
			str.remove_prefix(1);

			auto remaining_expression= ParseRegexpStringImpl(str);
			if(remaining_expression == std::nullopt)
				return std::nullopt;

			if(remaining_expression->size() == 1u)
			{
				if(const auto alternative= std::get_if<Alternatives>(&remaining_expression->front().el))
				{
					alternative->alternatives.insert(alternative->alternatives.begin(), std::move(chain));
					return std::move(*remaining_expression);
				}
			}

			Alternatives alternatives;
			alternatives.alternatives.push_back(std::move(chain));
			alternatives.alternatives.push_back(std::move(*remaining_expression));

			res.el = std::move(alternatives);
			return RegexpElementsChain{ std::move(res) };
		}

		case '(':
		{
			str.remove_prefix(1);
			auto sub_elements= ParseRegexpStringImpl(str);
			if(sub_elements == std::nullopt)
				return std::nullopt;

			if(str.empty() || str.front() != ')')
			{
				// TODO - handle error here
				return std::nullopt;
			}
			str.remove_prefix(1);

			res.el= BracketExpression{ std::move(*sub_elements) };
		}
			break;

		case ')':
			return chain;

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

				const char c= str.front();
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
					const char end_c= str.front();
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
		{
			chain.push_back(std::move(res));
			break;
		}

		// Try extract sequence.
		switch(str.front())
		{
		case '+':
			res.seq.min_elements= 1;
			res.seq.max_elements= std::numeric_limits<decltype(res.seq.max_elements)>::max();
			str.remove_prefix(1);
			break;

		case '*':
			res.seq.min_elements= 0;
			res.seq.max_elements= std::numeric_limits<decltype(res.seq.max_elements)>::max();
			str.remove_prefix(1);
			break;

		case '{':
			str.remove_prefix(1); // skip {

			// TODO - skip whitespaces inside {}

			res.seq.min_elements= 0;
			while(!str.empty() && str.front() >= '0' && str.front() <= '9')
			{
				res.seq.min_elements*= 10;
				res.seq.min_elements+= str.front() - '0';
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

				if(!str.empty() && str.front() >= '0' && str.front() <= '9')
				{
					res.seq.max_elements= 0;
					do
					{
						res.seq.max_elements*= 10;
						res.seq.max_elements+= str.front() - '0';
						str.remove_prefix(1);
					}while(!str.empty() && str.front() >= '0' && str.front() <= '9');
				}
				else
					res.seq.max_elements= std::numeric_limits<size_t>::max();
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

		chain.push_back(std::move(res));
	}

	return chain;
}

} // namespace

std::optional<RegexpElementsChain> ParseRegexpString(const std::string_view str)
{
	// Do internal parsing in UTF-32 format (with fixed codepoint size). Convert input string into UTF-32.

	std::u32string str_utf32;

	{
		str_utf32.resize(str.size()); // utf-8 string always has more elements than utf-32 string with same content.
		auto src_start= reinterpret_cast<const llvm::UTF8*>(str.data());
		const auto src_end= src_start + str.size();
		const auto target_start_initial= reinterpret_cast<llvm::UTF32*>(str_utf32.data());
		auto target_start= target_start_initial;
		const auto target_end= target_start + str_utf32.size();
		const auto res= llvm::ConvertUTF8toUTF32(&src_start, src_end, &target_start, target_end, llvm::strictConversion);

		if(res != llvm::conversionOK)
			return std::nullopt;

		str_utf32.resize(size_t(target_start - target_start_initial));
	}

	std::basic_string_view<CharType> s= str_utf32;
	return ParseRegexpStringImpl(s);
}

} // namespace RegPanzer
