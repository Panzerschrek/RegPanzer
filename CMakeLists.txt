cmake_minimum_required(VERSION 2.8)

project(RegPanzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LLVM
if(NOT DEFINED LLVM_SRC_DIR)
	message(FATAL_ERROR "LLVM_SRC_DIR not defined. You can define path to llvm library, using variable LLVM_SRC_DIR")
endif()

set(LLVM_ENABLE_RTTI ON CACHE BOOL "")

add_subdirectory(${LLVM_SRC_DIR} ${CMAKE_BINARY_DIR}/llvm)

# RegPanzer options

# llvm does not export definitions as target properties, so, retrieve compile definitions manually.
get_directory_property(LLVM_COMPILE_DEFINITIONS DIRECTORY ${LLVM_SRC_DIR} COMPILE_DEFINITIONS)
foreach(d ${LLVM_COMPILE_DEFINITIONS})
	add_definitions("-D${d}")
endforeach()

file(GLOB REG_LIB_SOURCES "RegPanzerLib/*")
add_library(RegPanzerLib ${REG_LIB_SOURCES})
target_include_directories(RegPanzerLib PUBLIC ${Boost_INCLUDE_DIRS})
