#pragma once
#include <memory>
#include <variant>
#include <vector>

namespace RegPanzer
{

using CharType= char;

struct AnySymbol{};

struct SpecificSymbol
{
	CharType code= '\0';
};

struct OneOf
{
	std::vector<CharType> variants;
	std::vector< std::pair<CharType, CharType> > ranges;
	bool inverse_flag= false;
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
	bool greedy= false;
};

struct RegexpElementFull
{
	std::variant<
		AnySymbol,
		SpecificSymbol,
		OneOf>
		el;

	Sequence seq;

	std::unique_ptr<RegexpElementFull> next;
};


// TODO - add conditionals

} // namespace RegPanzer
