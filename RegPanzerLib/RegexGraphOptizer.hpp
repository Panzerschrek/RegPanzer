#pragma once
#include "RegexGraph.hpp"

namespace RegPanzer
{

OneOf GetPossibleStartSybmols(const GraphElements::NodePtr& node);

// May return "true" even if symbols set actually are not intersected.
bool HasIntersection(const OneOf& l, const OneOf& r);

// Consumes input.
// Shared nodes are (partially) reused.
RegexGraphBuildResult OptimizeRegexGraph(RegexGraphBuildResult input_graph);

} // namespace RegPanzer
