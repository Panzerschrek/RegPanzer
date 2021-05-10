#pragma once
#include "RegexGraph.hpp"
#include <string_view>

namespace RegPanzer
{

// Match UTF-8 string.

// Returns 0 if found nothing, otherwise returns number of subpatetterns.
size_t Match(
	const GraphElements::NodePtr& node,
	std::string_view str,
	size_t start_pos,
	std::string_view* out_groups, /* 0 - whole pattern, 1 - first subpattern, etc.*/
	size_t out_groups_count /* size of ouptut array of groups */
	);

} // namespace RegPanzer
