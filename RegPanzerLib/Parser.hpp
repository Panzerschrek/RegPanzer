#pragma once
#include "RegexElements.hpp"
#include <string>
#include <string_view>

namespace RegPanzer
{

struct ParseError
{
	size_t pos= 0;
	std::string message;
};

using ParseErrors= std::vector<ParseError>;

using ParseResult= std::variant<RegexElementsChain, ParseErrors>;

// Parse regex in UTF-8 format.
ParseResult ParseRegexString(std::string_view str);

} // namespace RegPanzer
