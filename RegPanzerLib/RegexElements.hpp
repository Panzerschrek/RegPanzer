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

struct Group
{
	size_t index= std::numeric_limits<size_t>::max(); // Started from 1.
	RegexElementsChain elements;

	bool operator==(const Group& other) const { return this->index == other.index && this->elements == other.elements; }
	bool operator!=(const Group& other) const { return !(*this == other); }
};

struct BackReference
{
	size_t index= std::numeric_limits<size_t>::max(); // Started from 1.

	bool operator==(const BackReference& other) const { return this->index == other.index; }
	bool operator!=(const BackReference& other) const { return !(*this == other); }
};

struct NonCapturingGroup
{
	RegexElementsChain elements;

	bool operator==(const NonCapturingGroup& other) const { return this->elements == other.elements; }
	bool operator!=(const NonCapturingGroup& other) const { return !(*this == other); }
};

struct AtomicGroup
{
	RegexElementsChain elements;

	bool operator==(const AtomicGroup& other) const { return this->elements == other.elements; }
	bool operator!=(const AtomicGroup& other) const { return !(*this == other); }
};

struct Alternatives
{
	std::vector<RegexElementsChain> alternatives;

	bool operator==(const Alternatives& other) const { return alternatives == other.alternatives; }
	bool operator!=(const Alternatives& other) const { return !(*this == other); }
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

struct LineStartAssertion
{
	bool operator==(const LineStartAssertion&) const { return true; }
	bool operator!=(const LineStartAssertion& other) const { return !(*this == other); }
};

struct LineEndAssertion
{
	bool operator==(const LineEndAssertion&) const { return true; }
	bool operator!=(const LineEndAssertion& other) const { return !(*this == other); }
};

struct ConditionalElement
{
	Look look;
	Alternatives alternatives; // Expected exactly 2 alternatives.

	bool operator==(const ConditionalElement& other) const { return look == other.look && alternatives == other.alternatives; }
	bool operator!=(const ConditionalElement& other) const { return !(*this == other); }
};

struct SubroutineCall
{
	size_t index= std::numeric_limits<size_t>::max(); // 0 - whole expression, 1 - first grout, 2 - second group, etc.
	bool operator==(const SubroutineCall& other) const { return index == other.index; }
	bool operator!=(const SubroutineCall& other) const { return !(*this == other); }
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

struct RegexElementFull
{
	using ElementType =
		std::variant<
			AnySymbol,
			SpecificSymbol,
			OneOf,
			Group,
			BackReference,
			NonCapturingGroup,
			AtomicGroup,
			Alternatives,
			Look,
			LineStartAssertion,
			LineEndAssertion,
			ConditionalElement,
			SubroutineCall>;

	ElementType	el;

	Sequence seq;

	bool operator==(const RegexElementFull& other) const { return el == other.el && seq == other.seq; }
	bool operator!=(const RegexElementFull& other) const { return !(*this == other); }
};

} // namespace RegPanzer
