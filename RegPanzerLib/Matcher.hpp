#pragma once
#include "RegexGraph.hpp"
#include <optional>

namespace RegPanzer
{

// Match UTF-8 string.
using MatchResult= std::optional<std::string_view>;
MatchResult Match(const GraphElements::NodePtr& node, std::string_view str);

} // namespace RegPanzer
