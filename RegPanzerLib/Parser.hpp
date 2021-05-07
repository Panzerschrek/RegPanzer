#pragma once
#include "RegexElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

struct ParseError
{
	size_t pos;
	std::string message;
};

using ParseErrors= std::vector<ParseError>;

using ParseResult= std::variant<RegexElementsChain, ParseErrors>;

// Parse regex in UTF-8 format.
ParseResult ParseRegexString(std::string_view str);

} // namespace RegPanzer
