# ShiftingChecker

ShiftingChecker is a Clang Static Analyzer checker which checks for improper shifting: shifting by a negative value or by a value too large.

## Installation

1. Build [LLVM](https://github.com/llvm/llvm-project) following the instructions from the Github page. (**NOTE**: Over 8GiB of RAM *highly* recommended).
2. Copy `ShiftingChecker.cpp` to the `llvm-project/clang/lib/StaticAnalyzer/Checkers` folder.
3. Edit the `CMakeLists.txt` file in the aforementioned `Checkers` folder so that `cmake`could recognize added `ShiftingChecker.cpp` file.
4. Add following block of code into `Checkers.td` file contained in `llvm-project/clang/include/clang/StaticAnalyzer/Checkers`: 
```
let ParentPackage = UnixAlpha in {
    ...
    def ShiftingChecker : Checker<"ShiftingChecker">,  
                          HelpText<"Check for shifting by negative value or value too large">,  
                          Documentation<NotDocumented>;  
    ...
} // end "alpha.unix" 
```
5. In the `build` directory, run `make clang`.  This will only rebuild Clang (you can use this command for making any subsequent changes to the checker, as well).

## Usage

For applications written in C, run:

```
clang --analyze -Xanalyzer -analyzer-checker=unix,alpha example.c
```
For applications written in C++, run:
```
clang++ --analyze -Xanalyzer -analyzer-checker=unix,alpha example.cpp
```
Both `clang` and `clang++` can be found in `llvm-project/build/bin`.

## Testing

ShiftingChecker can be tested via `clang-lit` by running:

```
llvm-project/build/bin/llvm-lit <Absolute path to shifting-validation.c> -a
```
