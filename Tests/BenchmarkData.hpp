#pragma once
#include <string>

namespace RegPanzer
{

struct BenchmarkDataElement
{
	std::string regex_str;

	using DataGeneratorFunc= std::string (*)();

	DataGeneratorFunc data_generation_func;
};

extern const BenchmarkDataElement g_benchmark_data[];
extern const size_t g_benchmark_data_size;

} // namespace RegPanzer
