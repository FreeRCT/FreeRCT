#!/usr/bin/env python
# encoding: utf-8

"""Checks whether clang-tidy warnings that were previously cleaned have regressed."""

import re
import sys

# Checks we do not intend to clear now or ever.
PERM_SUPPRESSED_CHECKS = {
	'[cppcoreguidelines-pro-bounds-pointer-arithmetic]',  # do not use pointer arithmetic 
	'[cppcoreguidelines-pro-type-union-access]',  # do not access members of unions; use (boost::)variant instead 
	'[llvmlibc-callee-namespace]',  # 'operator!=' must resolve to a function declared within the '__llvm_libc' namespace 
	'[llvmlibc-implementation-in-namespace]',  # declaration must be declared within the '__llvm_libc' namespace 
	'[llvmlibc-restrict-system-libc-headers]',  # system include map not allowed 
	'[modernize-use-trailing-return-type]',  # use a trailing return type for this function 
	'[altera-id-dependent-backward-branch]',  # backward branch (for loop) is ID-dependent due to variable reference to 'i' and may cause performance degradation  
	'[misc-no-recursion]',  # function 'HandleImports' is within a recursive call chain 
	'[cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays]',  # do not declare C-style arrays, use std::array<> instead 
	'[cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay]',  # do not implicitly decay an array into a pointer; consider using gsl::array_view or an explicit cast instead 
	'[cppcoreguidelines-pro-type-vararg,hicpp-vararg]',  # do not call c-style vararg functions 
	'[cert-dcl50-cpp]',  # do not define a C-style variadic function; consider using a function parameter pack or currying instead 
	'[bugprone-suspicious-include]',  # suspicious #include of file with '.cpp' extension 
	'[altera-unroll-loops]',  # kernel performance could be improved by unrolling this loop with a '#pragma unroll' directive 
	'[cppcoreguidelines-pro-type-reinterpret-cast]',  # do not use reinterpret_cast 
}

# Checks we intend to clear in the future. When a check has been cleared, remove it from this list.
TEMP_SUPPRESSED_CHECKS = {
	'[altera-struct-pack-align]',  # accessing fields in struct 'MaskInformation' is inefficient due to poor alignment; currently aligned to 8 bytes, but recommended alignment is 32 bytes 
	'[bugprone-branch-clone]',  # switch has 3 consecutive identical branches 
	'[bugprone-easily-swappable-parameters]',  # 2 adjacent parameters of 'IsEmpty' of similar type ('int') are easily swapped by mistake 
	'[bugprone-implicit-widening-of-multiplication-result]',  # result of multiplication in type 'int' is used as a pointer offset after an implicit widening conversion to type 'ptrdiff_t' 
	'[bugprone-incorrect-roundings]',  # casting (double + 0.5) to integer leads to incorrect rounding; consider using lround (#include <cmath>) instead 
	'[bugprone-integer-division]',  # result of integer division used in a floating point context; possible loss of precision 
	'[bugprone-macro-parentheses]',  # macro argument should be enclosed in parentheses 
	'[bugprone-misplaced-widening-cast]',  # either cast from 'int' to 'yy_size_t' (aka 'unsigned long') is ineffective, or there is loss of precision before the conversion 
	'[bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions]',  # narrowing conversion from 'int16' (aka 'short') to signed type 'int8' (aka 'signed char') is implementation-defined 
	'[bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp]',  # declaration uses identifier '_dummy_position', which is reserved in the global namespace 
	'[bugprone-signed-char-misuse]',  # comparison between 'signed char' and 'unsigned char' 
	'[bugprone-signed-char-misuse,cert-str34-c]',  # 'signed char' to 'int' conversion; consider casting to 'unsigned char' first. 
	'[bugprone-string-integer-assignment]',  # an integer is interpreted as a character code when assigning it to a string; if this is intended, cast the integer to the appropriate character type; if you want a string representation, use the appropriate conversion facility 
	'[bugprone-too-small-loop-variable]',  # loop variable has narrower type 'int8' (aka 'signed char') than iteration's upper bound 'uint8' (aka 'unsigned char') 
	'[cert-dcl03-c,hicpp-static-assert,misc-static-assert]',  # found assert() that could be replaced by static_assert() 
	'[cert-dcl16-c,hicpp-uppercase-literal-suffix,readability-uppercase-literal-suffix]',  # integer literal has suffix 'll', which is not uppercase 
	'[cert-err33-c]',  # the value returned by this function should be used 
	'[cert-err34-c]',  # 'atoll' used to convert a string to an integer value, but function will not report conversion errors; consider using 'strtoll' instead 
	'[cert-err52-cpp]',  # do not call 'setjmp'; consider using exception handling instead 
	'[cert-err58-cpp]',  # initialization of '_dummy_position' with static storage duration may throw an exception that cannot be caught 
	'[cert-flp30-c]',  # loop induction expression should not have floating-point type 
	'[clang-analyzer-cplusplus.NewDelete]',  # Use of memory after it is freed 
	'[clang-analyzer-cplusplus.NewDeleteLeaks]',  # Potential memory leak 
	'[clang-analyzer-deadcode.DeadStores]',  # Value stored to 'new_index' during its initialization is never read 
	'[clang-analyzer-optin.cplusplus.UninitializedObject]',  # 3 uninitialized fields at the end of the constructor call 
	'[clang-analyzer-optin.cplusplus.VirtualCall]',  # Call to virtual method 'UnixDirectoryReader::ClosePath' during destruction bypasses virtual dispatch 
	'[clang-analyzer-security.FloatLoopCounter]',  # Variable 'x' with floating point type 'double' should not be used as a loop counter 
	'[clang-analyzer-security.insecureAPI.strcpy]',  # Call to function 'strcpy' is insecure as it does not provide bounding of the memory buffer. Replace unbounded copy functions with analogous functions that support length arguments such as 'strlcpy'. CWE-119 
	'[concurrency-mt-unsafe]',  # function is not thread safe 
	'[cppcoreguidelines-avoid-goto,hicpp-avoid-goto]',  # avoid using 'goto' for flow control 
	'[cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers]',  # 12 is a magic number; consider replacing it with a named constant 
	'[cppcoreguidelines-avoid-non-const-global-variables]',  # variable '_parsed_data' is non-const and globally accessible, consider making it const 
	'[cppcoreguidelines-explicit-virtual-functions,hicpp-use-override,modernize-use-override]',  # annotate this function with 'override' or (rarely) 'final' 
	'[cppcoreguidelines-init-variables]',  # variable 'ret' is not initialized 
	'[cppcoreguidelines-macro-usage]',  # macro 'YYBISON' used to declare a constant; consider using a 'constexpr' constant 
	'[cppcoreguidelines-no-malloc,hicpp-no-malloc]',  # do not manage memory manually; use RAII 
	'[cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes]',  # member variable 'ride_selector' has public visibility 
	'[cppcoreguidelines-owning-memory]',  # assigning newly created 'gsl::owner<>' to non-owner 'uint8 *' (aka 'unsigned char *') 
	'[cppcoreguidelines-prefer-member-initializer]',  # 'filename' should be initialized in an in-class default member initializer 
	'[cppcoreguidelines-pro-bounds-constant-array-index]',  # do not use array subscript when the index is not an integer constant expression 
	'[cppcoreguidelines-pro-type-cstyle-cast]',  # do not use C-style cast to convert between unrelated types 
	'[cppcoreguidelines-pro-type-member-init,hicpp-member-init]',  # constructor does not initialize these fields: save_index 
	'[cppcoreguidelines-pro-type-static-cast-downcast]',  # do not use static_cast to downcast from a base to a derived class; use dynamic_cast instead 
	'[cppcoreguidelines-special-member-functions,hicpp-special-member-functions]',  # class 'Values' defines a non-default destructor but does not define a copy constructor, a copy assignment operator, a move constructor or a move assignment operator 
	'[google-explicit-constructor,hicpp-explicit-conversions]',  # single-argument constructors must be marked explicit to avoid unintentional implicit conversions 
	'[google-readability-braces-around-statements,hicpp-braces-around-statements,readability-braces-around-statements]',  # statement should be inside braces 
	'[google-readability-casting]',  # C-style casts are discouraged; use static_cast/const_cast/reinterpret_cast 
	'[google-runtime-int]',  # consider replacing 'long long' with 'int64' 
	'[hicpp-braces-around-statements,readability-braces-around-statements]',  # statement should be inside braces 
	'[hicpp-deprecated-headers,modernize-deprecated-headers]',  # inclusion of deprecated C++ header 'stdio.h'; consider using 'cstdio' instead 
	'[hicpp-multiway-paths-covered]',  # potential uncovered code path; add a default label 
	'[hicpp-named-parameter,readability-named-parameter]',  # all parameters should be named in a function 
	'[hicpp-signed-bitwise]',  # use of a signed integer operand with a binary bitwise operator 
	'[hicpp-uppercase-literal-suffix]',  # floating point literal has suffix 'f', which is not uppercase 
	'[hicpp-uppercase-literal-suffix,readability-uppercase-literal-suffix]',  # floating point literal has suffix 'f', which is not uppercase 
	'[hicpp-use-auto,modernize-use-auto]',  # use auto when initializing with new to avoid duplicating the type name 
	'[hicpp-use-emplace,modernize-use-emplace]',  # use emplace_back instead of push_back 
	'[hicpp-use-equals-default,modernize-use-equals-default]',  # use '= default' to define a trivial default constructor 
	'[hicpp-use-nullptr,modernize-use-nullptr]',  # use nullptr 
	'[llvm-else-after-return,readability-else-after-return]',  # do not use 'else' after 'continue' 
	'[llvm-include-order]',  # #includes are not sorted properly 
	'[misc-non-private-member-variables-in-classes]',  # member variable 'name' has public visibility
	'[misc-unused-parameters]',  # parameter 'symbols' is unused 
	'[modernize-loop-convert]',  # use range-based for loop instead 
	'[modernize-make-unique]',  # use std::make_unique instead 
	'[modernize-pass-by-value]',  # pass by value and use std::move 
	'[modernize-raw-string-literal]',  # escaped string literal can be written as a raw string literal 
	'[modernize-redundant-void-arg]',  # redundant void argument list in function definition 
	'[modernize-return-braced-init-list]',  # avoid repeating the return type from the declaration; use a braced initializer list instead 
	'[modernize-use-bool-literals]',  # converting integer literal to bool, use bool literal instead 
	'[modernize-use-default-member-init]',  # use default member initializer for 'line_number' 
	'[modernize-use-nodiscard]',  # function 'GetMessage' should be marked [[nodiscard]] 
	'[modernize-use-nullptr]',  # use nullptr 
	'[modernize-use-using]',  # use 'using' instead of 'typedef' 
	'[performance-for-range-copy]',  # loop variable is copied but only used as const reference; consider making it a const reference 
	'[performance-implicit-conversion-in-loop]',  # the type of the loop variable 'bundle' is different from the one returned by the iterator and generates an implicit conversion; you can either change the type to the matching one ('const std::pair<const std::basic_string<char>, std::map<std::basic_string<char>, std::map<std::basic_string<char>, std::pair<std::basic_string<char>, Position>>>> &' but 'const auto&' is always a valid option) or remove the reference to make it explicit that you are creating a new value 
	'[performance-no-automatic-move]',  # constness of 'text' prevents automatic move 
	'[performance-no-int-to-ptr]',  # integer to pointer cast pessimizes optimization opportunities 
	'[performance-type-promotion-in-math-fn]',  # call to 'hypot' promotes float to double 
	'[performance-unnecessary-value-param]',  # the parameter 'fname' is copied for each invocation but only used as a const reference; consider making it a const reference 
	'[readability-avoid-const-params-in-decls]',  # parameter 'wid_num' is const-qualified in the function declaration; const-qualification of parameters only has an effect in function definitions 
	'[readability-container-data-pointer]',  # 'data' should be used for accessing the data pointer instead of taking the address of the 0-th element 
	'[readability-container-size-empty]',  # the 'empty' method should be used to check for emptiness instead of 'size' 
	'[readability-convert-member-functions-to-static]',  # method 'GetDefaultCarType' can be made static 
	'[readability-duplicate-include]',  # duplicate include 
	'[readability-function-cognitive-complexity]',  # function 'Encode' has cognitive complexity of 57 (threshold 25) 
	'[readability-identifier-length]',  # parameter name 'd' is too short, expected at least 3 characters 
	'[readability-implicit-bool-conversion]',  # implicit conversion 'int' -> bool 
	'[readability-inconsistent-declaration-parameter-name]',  # function 'FileWriter::AddBlock' has a definition with different parameter names 
	'[readability-isolate-declaration]',  # multiple declarations in a single statement reduces readability 
	'[readability-make-member-function-const]',  # method 'CheckEndSave' can be made const 
	'[readability-non-const-parameter]',  # pointer parameter 'corners' can be pointer to const 
	'[readability-qualified-auto]',  # 'auto &str' can be declared as 'const auto &str' 
	'[readability-redundant-declaration]',  # redundant 'yylex' declaration 
	'[readability-suspicious-call-argument]',  # 1st argument 'num' (passed to 'num') looks like it might be swapped with the 2nd, 'number' (passed to 'number') 
	'[readability-use-anyofallof]',  # replace loop by 'std::all_of()' 
}

# Any check not present in either of the above lists is expected to be clear; regressions are errors.

CHECK_REGEX = re.compile(r'[/\\_a-zA-Z0-9.:]+.*\[([A-Za-z0-9.-]+)\]$')

def main():
    """Checks whether clang-tidy warnings that were previously cleaned have
    regressed."""
    if len(sys.argv) == 2:
        print('########################################################')
        print('########################################################')
        print('###                                                  ###')
        print('###   Checking clang-tidy results                    ###')
        print('###                                                  ###')
        print('########################################################')
        print('########################################################')
    else:
        print(
            'Usage: check_clang_tidy_results.py <log_file>')
        return 1

    log_file = sys.argv[1]

    errors = 0
    temp_suppressed = 0

    with open(log_file) as checkme:
        contents = checkme.readlines()
        for line in contents:
            if 'clang-analyzer-alpha' in line or '/rcdgen/parser.' in line or '/rcdgen/scanner.' in line:
                continue
            check_suppressed = False
            for check in PERM_SUPPRESSED_CHECKS:
                if check in line:
                    check_suppressed = True
                    break
            if not check_suppressed:
                for check in TEMP_SUPPRESSED_CHECKS:
                    if check in line:
                        check_suppressed = True
                        temp_suppressed = temp_suppressed + 1
                        break
            if not check_suppressed and CHECK_REGEX.match(line):
                print(line.strip())
                errors = errors + 1

    if errors > 0:
        print('########################################################')
        print('########################################################')
        print('###                                                  ###')
        print('###   Found %s error(s)                               ###'
              % errors)
        print('###                                                  ###')
        print('########################################################')
        print('########################################################')
        print('Information about the checks can be found on:')
        print('https://clang.llvm.org/extra/clang-tidy/checks/list.html')
        return 1

    print('###                                                  ###')
    print('###   Check has passed (%d checks currently ignored)  ###' % temp_suppressed)
    print('###                                                  ###')
    print('########################################################')
    print('########################################################')
    return 0


if __name__ == '__main__':
    sys.exit(main())
