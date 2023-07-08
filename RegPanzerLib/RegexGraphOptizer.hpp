#pragma once
#include "RegexGraph.hpp"

namespace RegPanzer
{

// Consumes input.
// Shared nodes are (partially) reused.
RegexGraphBuildResult OptimizeRegexGraph(RegexGraphBuildResult input_graph);

} // namespace RegPanzer
