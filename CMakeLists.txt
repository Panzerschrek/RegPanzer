cmake_minimum_required(VERSION 3.10)

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
llvm_map_components_to_libnames(LLVM_LIBS_FOR_REG_PANZER_TEST Interpreter)

# RegPanzerLib

file(GLOB REG_LIB_SOURCES "RegPanzerLib/*")
add_library(RegPanzerLib ${REG_LIB_SOURCES})
target_include_directories(RegPanzerLib PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(RegPanzerLib PUBLIC ${LLVM_LIBS_FOR_REG_PANZER_LIB})

# Tests
# Tests depends on gtest library which present only in LLVM sources but not in LLVM binary distribution.

if(DEFINED LLVM_SRC_DIR)
	file(GLOB REG_PANZER_TEST_SOURCES "Tests/*")
	add_executable(RegPanzerTest ${REG_PANZER_TEST_SOURCES})
	target_link_libraries(RegPanzerTest PRIVATE gtest gtest_main ${LLVM_LIBS_FOR_REG_PANZER_TEST})
	target_include_directories(RegPanzerTest PRIVATE ${LLVM_SRC_DIR}/utils/unittest/googletest/include/)
	add_custom_command(TARGET RegPanzerTest POST_BUILD COMMAND RegPanzerTest)
	target_compile_definitions(RegPanzerTest PRIVATE -DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0)
endif()
