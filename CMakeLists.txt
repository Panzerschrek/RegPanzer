cmake_minimum_required(VERSION 3.10)

project(RegPanzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS Off)

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

# RegPanzer options

file(GLOB REG_LIB_SOURCES "RegPanzerLib/*")
add_library(RegPanzerLib ${REG_LIB_SOURCES})
target_include_directories(RegPanzerLib PUBLIC ${Boost_INCLUDE_DIRS})
