# RegPanzer - library and compiler for regular expressions

**RegPanzer** library allows you to produce native match function for specified regular expression.

**RegPanzerCompiler** allows you to produce object file with such function.

RegPanzer uses PRCE regular expressions syntax.
It is still in experimental stage, not all PCRE features is implemented now.


## Basic usage

Produce matcher function using *RegPanzerCompiler*:

```
RegPanzerCompiler "[a-z]+[0-9]+" -o test.o --function-name=MatchWordWithNumber
```

Create header file for your language (C++, fro example):
```cpp
size_t MatchWordWithNumber(
    const char* str,
    size_t str_size,
    size_t start_offset,
    size_t* out_subpatterns,
    size_t number_of_subpatterns);
```

Call this function from your program to perform match for your regular expression, link the object file (test.o) against your program.


## How to build

Download/install LLVM library (LLVM 15.0.7 used in this project).

Run _cmake_ for _CmakeListst.txt_ in repository root to generate project for your favorite build system or IDE.
Specify LLVM_SRC_DIR cmake variable if you wish to build RegPanzer based on LLVM library in source form OR specify LLVM_LIB_DIR variable to build *RegPanzer* using binary LLVM distribution.

Note that tests/benchamrk build supported only with LLVM in source form.

## Authors

Copyright © 2021-2023 Artöm "Panzerscrek" Kunç.
