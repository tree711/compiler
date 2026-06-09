#!/bin/bash

# TinyC 编译器自动化测试脚本

echo "========================================"
echo "  TinyC 编译器自动化测试"
echo "========================================"
echo ""

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 测试计数
TOTAL=0
PASSED=0
FAILED=0

# 测试函数
run_test() {
    local test_name=$1
    local test_file=$2
    local output_file=$3
    
    TOTAL=$((TOTAL + 1))
    
    echo "---------- 测试 $TOTAL: $test_name ----------"
    
    if [ ! -f "$test_file" ]; then
        echo -e "${RED}✗ 测试文件不存在: $test_file${NC}"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # 运行编译器
    if ./compiler "$test_file" -o "$output_file" 2>&1 > /dev/null; then
        echo -e "${GREEN}✓ 编译成功${NC}"
        
        # 检查生成的汇编文件
        if [ -f "$output_file" ]; then
            echo -e "${GREEN}✓ 汇编文件生成成功: $output_file${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗ 汇编文件未生成${NC}"
            FAILED=$((FAILED + 1))
        fi
    else
        echo -e "${RED}✗ 编译失败${NC}"
        FAILED=$((FAILED + 1))
    fi
    
    echo ""
}

# 检查编译器是否存在
if [ ! -f "./compiler" ]; then
    echo "编译器不存在，正在编译..."
    g++ -std=c++17 -Iinclude src/main.cpp src/Lexer.cpp src/Parser.cpp \
        src/Semantic.cpp src/IRGenerator.cpp src/Optimizer.cpp \
        src/CodeGenerator.cpp -o compiler
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}编译器编译失败！${NC}"
        exit 1
    fi
    echo -e "${GREEN}编译器编译成功！${NC}"
    echo ""
fi

# 运行测试
run_test "简单算术运算" "test1_arithmetic.c" "test1.s"
run_test "嵌套 if-else" "test2_nested_if.c" "test2.s"
run_test "嵌套 while 循环" "test3_nested_while.c" "test3.s"
run_test "复杂表达式" "test4_complex_expr.c" "test4.s"
run_test "比较运算符" "test5_comparison.c" "test5.s"
run_test "边界情况" "test6_edge_cases.c" "test6.s"
run_test "原始测试用例" "test.c" "test_original.s"

# 输出测试结果
echo "========================================"
echo "  测试结果统计"
echo "========================================"
echo "总测试数: $TOTAL"
echo -e "通过: ${GREEN}$PASSED${NC}"
echo -e "失败: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ 所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}✗ 有 $FAILED 个测试失败${NC}"
    exit 1
fi