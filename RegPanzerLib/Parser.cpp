#include "Parser.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/Support/ConvertUTF.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

using StrView= std::basic_string_view<CharType>;

std::optional<RegexElementsChain> ParseRegexStringImpl(size_t& next_group_index, StrView& str);

std::optional<RegexElementFull::ElementType> ParseEscapeSequence(StrView& str)
{
	str.remove_prefix(1); // Remove '\'

	if(str.empty())
		return std::nullopt;

	const CharType c= str.front();
	str.remove_prefix(1);

	switch(c)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '(':
	case ')':
	case '^':
	case '$':
	case '.':
	case '*':
	case '+':
	case '|':
	case '?':
	case '\\':
	case '/':
		return SpecificSymbol{ c };

	case 'd':
	case 'D':
		return OneOf{ {}, { {'0', '9'} }, c == 'D' };

	case 'w':
	case 'W':
		return OneOf{ { '_' }, { {'a', 'z'}, {'A', 'Z'}, {'0', '9'} }, c == 'W' };

	case 's':
	case 'S':
		return OneOf{ { ' ', '\r', '\n', '\t', '\f', '\v', 0x00A0, 0x1680, 0x2028, 0x2029, 0x202F, 0x3000, 0xFEFF }, { {0x2000, 0x200A} }, c == 'S' };

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return BackReference{ size_t( c - '0' ) };

	case 'p':
		// TODO - parse special symbol classes here.
		return std::nullopt;
	}

	return std::nullopt;
}

std::optional<OneOf> ParseOneOf(StrView& str)
{
	str.remove_prefix(1); // Remove [

	OneOf one_of;

	if(!str.empty() && str.front() == '^')
	{
		str.remove_prefix(1); // Remove ^
		one_of.inverse_flag= true;
	}

	while(!str.empty() && str.front() != ']')
	{
		constexpr StrView word_class = U"[:word:]";
		if(str.size() >= word_class.size() && str.substr(0, word_class.size()) == word_class)
		{
			str.remove_prefix(word_class.size());
			one_of.variants.push_back('_');
			one_of.ranges.emplace_back('a', 'z');
			one_of.ranges.emplace_back('A', 'Z');
			one_of.ranges.emplace_back('0', '9');
			continue;
		}

		constexpr StrView not_word_class = U"^[:word:]";
		if(str.size() >= not_word_class.size() && str.substr(0, not_word_class.size()) == not_word_class)
		{
			str.remove_prefix(not_word_class.size());
			one_of.inverse_flag= true;
			one_of.variants.push_back('_');
			one_of.ranges.emplace_back('a', 'z');
			one_of.ranges.emplace_back('A', 'Z');
			one_of.ranges.emplace_back('0', '9');
			continue;
		}

		CharType c= str.front();

		if(c == '\\')
		{
			if(const auto el= ParseEscapeSequence(str))
			{
				if(const auto specific_symbol= std::get_if<SpecificSymbol>(&*el))
					c= specific_symbol->code;
				else if(const auto inner_one_of= std::get_if<OneOf>(&*el))
				{
					// TODO - handle cases with invert flag for specific parts of "oneOf".
					one_of.inverse_flag= inner_one_of->inverse_flag;
					one_of.variants.insert(one_of.variants.end(), inner_one_of->variants.begin(), inner_one_of->variants.end());
					one_of.ranges.insert(one_of.ranges.end(), inner_one_of->ranges.begin(), inner_one_of->ranges.end());
					continue;
				}
				else
					return std::nullopt;
			}
			else
				return std::nullopt;
		}
		else
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

	return one_of;
}

SequenceMode ParseSequenceMode(StrView& str)
{
	if(str.empty())
		return SequenceMode::Greedy;

	switch(str.front())
	{
	case '?':
		str.remove_prefix(1);
		return SequenceMode::Lazy;

	case '+':
		str.remove_prefix(1);
		return SequenceMode::Possessive;
	};

	return SequenceMode::Greedy;
}

std::optional<Sequence> ParseSequence(StrView& str)
{
	Sequence seq;
	seq.min_elements= 1;
	seq.max_elements= 1;

	if(str.empty())
		return seq;

	// Try extract sequence.
	switch(str.front())
	{
	case '+':
		seq.min_elements= 1;
		seq.max_elements= std::numeric_limits<decltype(seq.max_elements)>::max();
		str.remove_prefix(1);
		seq.mode= ParseSequenceMode(str);
		break;

	case '*':
		seq.min_elements= 0;
		seq.max_elements= std::numeric_limits<decltype(seq.max_elements)>::max();
		str.remove_prefix(1);
		seq.mode= ParseSequenceMode(str);
		break;

	case '?':
		seq.min_elements= 0;
		seq.max_elements= 1;
		str.remove_prefix(1);
		seq.mode= ParseSequenceMode(str);
		break;

	case '{':
		str.remove_prefix(1); // skip {

		// TODO - skip whitespaces inside {}

		seq.min_elements= 0;
		while(!str.empty() && str.front() >= '0' && str.front() <= '9')
		{
			seq.min_elements*= 10;
			seq.min_elements+= str.front() - '0';
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
				seq.max_elements= 0;
				do
				{
					seq.max_elements*= 10;
					seq.max_elements+= str.front() - '0';
					str.remove_prefix(1);
				}while(!str.empty() && str.front() >= '0' && str.front() <= '9');
			}
			else
				seq.max_elements= Sequence::c_max;
		}
		else
			seq.max_elements= seq.min_elements;

		if(str.empty() || str.front() != '}')
		{
			// TODO - handle error here
			return std::nullopt;
		}
		str.remove_prefix(1); // Skip }

		seq.mode= ParseSequenceMode(str);

		break;

	default:
		break;
	}

	return seq;
}

std::optional<Look> ParseLook(size_t& next_group_index, StrView& str)
{
	Look look;

	if(str.empty())
		return std::nullopt;

	if(str.front() == '<')
	{
		str.remove_prefix(1);
		look.forward= false;
	}
	else
		look.forward= true;

	if(str.empty())
		return std::nullopt;

	if(str.front() == '=')
		look.positive= true;
	else if(str.front() == '!')
		look.positive= false;
	else
		return std::nullopt;

	str.remove_prefix(1);

	auto sub_elements= ParseRegexStringImpl(next_group_index, str);
	if(sub_elements == std::nullopt)
		return std::nullopt;

	if(str.empty() || str.front() != ')')
		return std::nullopt;
	str.remove_prefix(1);

	look.elements= std::move(*sub_elements);
	return look;
}

std::optional<RegexElementsChain> ParseRegexStringImpl(size_t& next_group_index, StrView& str)
{
	RegexElementsChain chain;

	while(!str.empty())
	{
		RegexElementFull res;

		// Extract element.
		// TODO - porcess escape sequences.
		switch(str.front())
		{
		case '|':
		{
			str.remove_prefix(1);

			auto remaining_expression= ParseRegexStringImpl(next_group_index, str);
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
			res.seq.min_elements= 1;
			res.seq.max_elements= 1;

			return RegexElementsChain{ std::move(res) };
		}

		case '(':
			str.remove_prefix(1);

			if(str.empty())
				return std::nullopt;

			if(str.front() == '?')
			{
				str.remove_prefix(1);
				if(str.empty())
					return std::nullopt;

				if(str.front() == 'R')
				{
					str.remove_prefix(1);

					if(str.empty() || str.front() != ')')
						return std::nullopt;
					str.remove_prefix(1);

					res.el= SubroutineCall{ 0 };
				}
				else if(str.front() >= '0' && str.front() <= '9')
				{
					const auto index= size_t(str.front() - '0');
					str.remove_prefix(1);

					if(str.empty() || str.front() != ')')
						return std::nullopt;
					str.remove_prefix(1);

					res.el= SubroutineCall{ index };
				}
				else if(str.front() == ':')
				{
					str.remove_prefix(1);

					auto sub_elements= ParseRegexStringImpl(next_group_index, str);
					if(sub_elements == std::nullopt)
						return std::nullopt;

					if(str.empty() || str.front() != ')')
						return std::nullopt;
					str.remove_prefix(1);

					res.el= NonCapturingGroup{ std::move(*sub_elements) };
				}
				else if(str.front() == '>')
				{
					str.remove_prefix(1);

					auto sub_elements= ParseRegexStringImpl(next_group_index, str);
					if(sub_elements == std::nullopt)
						return std::nullopt;

					if(str.empty() || str.front() != ')')
						return std::nullopt;
					str.remove_prefix(1);

					res.el= AtomicGroup{ std::move(*sub_elements) };
				}
				else if(str.size() >= 2 && str[0] == '(' && str[1] == '?')
				{
					str.remove_prefix(2);
					auto look= ParseLook(next_group_index, str);
					if(look == std::nullopt)
						return std::nullopt;

					auto sub_elements= ParseRegexStringImpl(next_group_index, str);
					if(str.empty() || str.front() != ')')
						return std::nullopt;
					str.remove_prefix(1);

					if(sub_elements == std::nullopt || sub_elements->size() != 1)
						return std::nullopt;
					auto* const alternatives= std::get_if<Alternatives>(&sub_elements->front().el);
					if(alternatives == nullptr || alternatives->alternatives.size() != 2)
						return std::nullopt;

					ConditionalElement conditional_element;
					conditional_element.look= *look;
					conditional_element.alternatives= std::move(*alternatives);

					res.el= std::move(conditional_element);
				}
				else
				{
					auto look= ParseLook(next_group_index, str);
					if(look == std::nullopt)
						return std::nullopt;

					res.el= std::move(*look);
				}
			}
			else
			{
				const size_t current_group_index= next_group_index;
				++next_group_index;

				auto sub_elements= ParseRegexStringImpl(next_group_index, str);
				if(sub_elements == std::nullopt)
					return std::nullopt;

				if(str.empty() || str.front() != ')')
				{
					// TODO - handle error here
					return std::nullopt;
				}
				str.remove_prefix(1);

				res.el= Group{ current_group_index, std::move(*sub_elements) };
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
			auto one_of= ParseOneOf(str);
			if(one_of == std::nullopt)
				return std::nullopt;
			res.el= std::move(*one_of);
		}
			break;

		case '\\':
		{
			auto symbol= ParseEscapeSequence(str);
			if(symbol == std::nullopt)
				return std::nullopt;
			res.el= std::move(*symbol);
		}
			break;

		default:
			res.el= SpecificSymbol{ str.front() };
			str.remove_prefix(1);
			break;
		};

		auto seq= ParseSequence(str);
		if(seq == std::nullopt)
			return std::nullopt;
		res.seq= *seq;

		chain.push_back(std::move(res));
	}

	return chain;
}

} // namespace

std::optional<RegexElementsChain> ParseRegexString(const std::string_view str)
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

	StrView s= str_utf32;
	size_t next_group_index= 1;
	return ParseRegexStringImpl(next_group_index, s);
}

} // namespace RegPanzer
