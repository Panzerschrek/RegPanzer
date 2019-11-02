#pragma once
#include <variant>
#include <vector>

namespace PanzerReg
{

namespace RegexpElements
{

using CharType= char;

struct SpecificSymbol
{
	CharType code= '\0';
};

struct OneOf
{
	std::vector<CharType> variants;
	std::vector< std::pair<CharType, CharType> > ranges;
	// TODO - add inverse condition
	// TODO - any character flag
};

struct Sequence
{
	// * - min=0 max=uint_max
	// + - min=1, max=uint_max
	// {5, 10} - min= 5, max= 10
	// {7,} - min= 7, max=uint_max
	// ? - min=0, max=1
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	// TODO - greed specifier.
};

// TODO - add conditionals

} // namespace RegexpElements

using RegexpElement=
	std::variant<
		RegexpElements::SpecificSymbol,
		RegexpElements::OneOf,
		RegexpElements::Sequence>;

using Regexp= std::vector<RegexpElement>;

} // namespace PanzerReg
