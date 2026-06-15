#!/usr/bin/env bash

set -u

CXX=${CXX:-g++}
COMPILER=./compiler
TOTAL=0
PASSED=0
FAILED=0

build_compiler() {
    "$CXX" -std=c++17 -Wall -Wextra -Iinclude \
        src/main.cpp src/Lexer.cpp src/Parser.cpp src/Semantic.cpp \
        src/IRGenerator.cpp src/Optimizer.cpp src/CodeGenerator.cpp \
        -o "$COMPILER"
}

record_result() {
    local name=$1
    local result=$2
    TOTAL=$((TOTAL + 1))
    if [ "$result" -eq 0 ]; then
        echo "[PASS] $name"
        PASSED=$((PASSED + 1))
    else
        echo "[FAIL] $name"
        FAILED=$((FAILED + 1))
    fi
}

run_valid_test() {
    local source=$1
    local mode=$2
    local stem
    stem=$(basename "$source" .c)
    local asm="${stem}_${mode}.s"
    local exe="${stem}_${mode}.exe"
    local optimize=()

    if [ "$mode" = "optimized" ]; then
        optimize=(-O)
    fi

    if ! "$COMPILER" "$source" -o "$asm" "${optimize[@]}" >/dev/null 2>&1; then
        record_result "$source ($mode): compile" 1
        return
    fi
    if ! "$CXX" "$asm" -o "$exe" >/dev/null 2>&1; then
        record_result "$source ($mode): assemble/link" 1
        return
    fi
    if ! timeout 3s "./$exe" >/dev/null 2>&1; then
        record_result "$source ($mode): run" 1
        return
    fi

    record_result "$source ($mode)" 0
}

run_invalid_test() {
    local source=$1
    local asm
    asm="$(basename "$source" .c)_invalid.s"

    if "$COMPILER" "$source" -o "$asm" >/dev/null 2>&1; then
        record_result "$source: expected rejection" 1
    else
        record_result "$source: expected rejection" 0
    fi
}

if ! build_compiler; then
    echo "Compiler build failed."
    exit 1
fi

VALID_TESTS=(
    test.c
    test1_arithmetic.c
    test2_nested_if.c
    test3_nested_while.c
    test4_complex_expr.c
    test5_comparison.c
    test6_edge_cases.c
)

for test_file in "${VALID_TESTS[@]}"; do
    run_valid_test "$test_file" normal
    run_valid_test "$test_file" optimized
done

run_invalid_test test_invalid_syntax.c
run_invalid_test test_invalid_undefined.c
run_invalid_test test_invalid_redeclaration.c
run_invalid_test test_invalid_divzero.c

echo
echo "Total: $TOTAL  Passed: $PASSED  Failed: $FAILED"
exit "$FAILED"
