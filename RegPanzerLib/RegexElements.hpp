#pragma once
#include <memory>
#include <variant>
#include <vector>

namespace RegPanzer
{

using CharType= char32_t;

struct RegexElementFull;
using RegexElementsChain= std::vector<RegexElementFull>;

struct AnySymbol
{
	bool operator==(const AnySymbol&) const { return true; }
	bool operator!=(const AnySymbol& other) const { return !(*this == other); }
};

struct SpecificSymbol
{
	CharType code= '\0';

	bool operator==(const SpecificSymbol& other) const { return code == other.code; }
	bool operator!=(const SpecificSymbol& other) const { return !(*this == other); }
};

struct OneOf
{
	std::vector<CharType> variants;
	std::vector< std::pair<CharType, CharType> > ranges;
	bool inverse_flag= false;

	bool operator==(const OneOf& other) const
	{
		return
			this->variants == other.variants &&
			this->ranges == other.ranges &&
			this->inverse_flag == other.inverse_flag;
	}

	bool operator!=(const OneOf& other) const { return !(*this == other); }
};

struct Look
{
	bool forward= true;
	bool positive= true;
	RegexElementsChain elements;

	bool operator==(const Look& other) const
	{
		return
			this->forward == other.forward &&
			this->positive == other.positive &&
			this->elements == other.elements;
	}

	bool operator!=(const Look& other) const { return !(*this == other); }
};

enum class SequenceMode : uint8_t
{
	Greedy,
	Lazy,
	Possessive,
};

struct Sequence
{
	static constexpr size_t c_max= std::numeric_limits<size_t>::max();

	// * - min=0 max=uint_max
	// + - min=1, max=uint_max
	// {5, 10} - min= 5, max= 10
	// {7,} - min= 7, max=uint_max
	// ? - min=0, max=1
	size_t min_elements= 0u;
	size_t max_elements= 0u;
	SequenceMode mode= SequenceMode::Greedy;

	bool operator==(const Sequence& other) const
	{
		return
			this->min_elements == other.min_elements &&
			this->max_elements == other.max_elements &&
			this->mode == other.mode;
	}

	bool operator!=(const Sequence& other) const { return !(*this == other); }
};

struct BracketExpression
{
	RegexElementsChain elements;

	bool operator==(const BracketExpression& other) const { return elements == other.elements; }
	bool operator!=(const BracketExpression& other) const { return !(*this == other); }
};

struct Alternatives
{
	std::vector<RegexElementsChain> alternatives;

	bool operator==(const Alternatives& other) const { return alternatives == other.alternatives; }
	bool operator!=(const Alternatives& other) const { return !(*this == other); }
};

struct RegexElementFull
{
	using ElementType =
		std::variant<
			AnySymbol,
			SpecificSymbol,
			BracketExpression,
			Alternatives,
			OneOf,
			Look>;

	ElementType	el;

	Sequence seq;

	bool operator==(const RegexElementFull& other) const { return el == other.el && seq == other.seq; }
	bool operator!=(const RegexElementFull& other) const { return !(*this == other); }
};

} // namespace RegPanzer
