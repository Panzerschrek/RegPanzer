cmake_minimum_required(VERSION 3.10)

option( REG_PANZER_BUILD_BENCHMARK "Build RegPanzer Benchmark" YES )

project(RegPanzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS Off)
if( MSVC )
	add_definitions( -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS )
endif()

# LLVM

if(DEFINED LLVM_SRC_DIR)
	message(STATUS "Build LLVM from sources")

	add_subdirectory(${LLVM_SRC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/llvm)

	include_directories(${LLVM_SRC_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/llvm/include)

	# llvm does not export definitions as target properties, so, retrieve compile definitions manually.
	get_directory_property(LLVM_COMPILE_DEFINITIONS DIRECTORY ${LLVM_SRC_DIR} COMPILE_DEFINITIONS)
	foreach( d ${LLVM_COMPILE_DEFINITIONS} )
		add_definitions( "-D${d}" )
	endforeach()

elseif(DEFINED LLVM_LIB_DIR)
	message(STATUS "Use binary LLVM package")

	set(CMAKE_PREFIX_PATH
		${CMAKE_PREFIX_PATH}
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang)

	set(CMAKE_MODULE_PATH
		${CMAKE_MODULE_PATH}
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang)

	find_package(LLVM REQUIRED)
	include_directories(${LLVM_INCLUDE_DIRS})

else()
	message(FATAL_ERROR "llvm not found. Define LLVM_SRC_DIR for building llvm from sources, or define LLVM_LIB_DIR for installed llvm.")
endif()

llvm_map_components_to_libnames(LLVM_LIBS_FOR_REG_PANZER_LIB Core)
llvm_map_components_to_libnames(LLVM_LIBS_FOR_REG_PANZER_COMPILER IPO ${LLVM_TARGETS_TO_BUILD})
llvm_map_components_to_libnames(LLVM_LIBS_FOR_REG_PANZER_TEST Interpreter MCJIT ${LLVM_TARGETS_TO_BUILD})

if(REG_PANZER_BUILD_BENCHMARK)
	add_subdirectory(google_benchmark)
endif()

# PCRE library used for cross-validation and benchmarks.
find_library(PCRE_LIBRARY pcre)
if(PCRE_LIBRARY)
	message("Found PCRE library: ${PCRE_LIBRARY}")
else()
	message("PCRE library not found. Cross-tests with this library will be disabled.")
endif()

# Setup our project compiler options.

if(NOT WIN32)
	add_compile_options( -fPIC )
endif()

if(MSVC)
	add_compile_options(/utf-8 /W3 /WX)

	if(NOT LLVM_ENABLE_RTTI)
		add_compile_options(/GR-)
	endif()
else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
	add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)

	if(NOT LLVM_ENABLE_RTTI)
		add_compile_options(-fno-rtti)
	endif()

	list(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections")
	list(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections")
endif()

# RegPanzerLib

file(GLOB_RECURSE REG_LIB_SOURCES "RegPanzerLib/*")
add_library(RegPanzerLib ${REG_LIB_SOURCES})
target_include_directories(RegPanzerLib PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(RegPanzerLib PUBLIC ${LLVM_LIBS_FOR_REG_PANZER_LIB})

# Compiler
file(GLOB COMPILER_SOURCES "Compiler/*")
add_executable(RegPanzerCompiler ${COMPILER_SOURCES})
target_link_libraries(RegPanzerCompiler PRIVATE RegPanzerLib ${LLVM_LIBS_FOR_REG_PANZER_COMPILER})

# Tests
# Tests depends on gtest library which present only in LLVM sources but not in LLVM binary distribution.

if(DEFINED LLVM_SRC_DIR)
	#
	# Tests
	#
	file(GLOB REG_PANZER_TEST_SOURCES "Tests/*")
	if(NOT PCRE_LIBRARY)
		list(REMOVE_ITEM REG_PANZER_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Tests/PcreRegexTest.cpp)
	endif()

	add_executable(RegPanzerTest ${REG_PANZER_TEST_SOURCES})
	target_link_libraries(RegPanzerTest PRIVATE RegPanzerLib llvm_gtest llvm_gtest_main ${LLVM_LIBS_FOR_REG_PANZER_TEST})
	target_include_directories(RegPanzerTest PRIVATE ${LLVM_SRC_DIR}/utils/unittest/googletest/include/)
	target_compile_definitions(RegPanzerTest PRIVATE -DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0)
	add_dependencies(RegPanzerTest RegPanzerCompiler) # Compiler is executed inside tests.

	if(PCRE_LIBRARY)
		target_link_libraries(RegPanzerTest PRIVATE ${PCRE_LIBRARY})
	endif()

	add_custom_command(TARGET RegPanzerTest POST_BUILD COMMAND RegPanzerTest)

endif()

#
# benchmark
#
if(REG_PANZER_BUILD_BENCHMARK)
	file(GLOB REG_PANZER_BENCHMARK_SOURCES "Benchmark/*")
	if(NOT PCRE_LIBRARY)
		list(REMOVE_ITEM REG_PANZER_BENCHMARK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Benchmark/PcreBenchmark.cpp)
	endif()

	add_executable(RegPanzerBenchmark ${REG_PANZER_BENCHMARK_SOURCES})
	target_link_libraries(RegPanzerBenchmark PRIVATE RegPanzerLib benchmark benchmark_main ${LLVM_LIBS_FOR_REG_PANZER_TEST})
	target_include_directories(RegPanzerBenchmark PRIVATE ${LLVM_SRC_DIR}/utils/benchmark/include/)

	if(PCRE_LIBRARY)
		target_link_libraries(RegPanzerBenchmark PRIVATE ${PCRE_LIBRARY})
	endif()
endif()
