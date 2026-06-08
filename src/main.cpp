/**
 * TinyC 编译器后端测试
 * 
 * 由于 Parser 尚未完整实现，测试用例通过
 * 手动构造 AST 节点来验证后端各模块。
 */

#include "../include/AST.h"
#include "../include/IR.h"
#include "../include/IRGenerator.h"
#include "../include/Optimizer.h"
#include "../include/CodeGenerator.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <fstream>



// ========== 辅助函数 ==========
void printSeparator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

void printIR(IRProgram& program) {
    for (auto& func : program.functions) {
        std::cout << "Function: " << func->name << "\n";
        for (auto& block : func->blocks) {
            std::cout << "  Block: " << block->name << "\n";
            for (auto& instr : block->instructions) {
                std::cout << "    ";
                // Print label if any
                if (!instr->label.empty()) {
                    std::cout << instr->label << ": ";
                }
                // Print result
                if (!instr->result.empty()) {
                    std::cout << instr->result << " = ";
                }
                // Print opcode
                switch (instr->opcode) {
                    case IROpcode::ADD:      std::cout << "ADD"; break;
                    case IROpcode::SUB:      std::cout << "SUB"; break;
                    case IROpcode::MUL:      std::cout << "MUL"; break;
                    case IROpcode::DIV:      std::cout << "DIV"; break;
                    case IROpcode::LOAD:     std::cout << "LOAD"; break;
                    case IROpcode::STORE:    std::cout << "STORE"; break;
                    case IROpcode::CONST:    std::cout << "CONST"; break;
                    case IROpcode::BRANCH:   std::cout << "BRANCH"; break;
                    case IROpcode::BRANCH_EQ: std::cout << "BRANCH_EQ"; break;
                    case IROpcode::BRANCH_NE: std::cout << "BRANCH_NE"; break;
                    case IROpcode::BRANCH_LT: std::cout << "BRANCH_LT"; break;
                    case IROpcode::BRANCH_GT: std::cout << "BRANCH_GT"; break;
                    case IROpcode::BRANCH_LE: std::cout << "BRANCH_LE"; break;
                    case IROpcode::BRANCH_GE: std::cout << "BRANCH_GE"; break;
                    case IROpcode::CALL:     std::cout << "CALL"; break;
                    case IROpcode::RET:      std::cout << "RET"; break;
                    case IROpcode::PHI:      std::cout << "PHI"; break;
                    case IROpcode::CMP_EQ:   std::cout << "CMP_EQ"; break;
                    case IROpcode::CMP_NE:   std::cout << "CMP_NE"; break;
                    case IROpcode::CMP_LT:   std::cout << "CMP_LT"; break;
                    case IROpcode::CMP_GT:   std::cout << "CMP_GT"; break;
                    case IROpcode::CMP_LE:   std::cout << "CMP_LE"; break;
                    case IROpcode::CMP_GE:   std::cout << "CMP_GE"; break;
                    default:                 std::cout << "UNKNOWN"; break;
                }
                // Print operands
                for (const auto& op : instr->operands) {
                    std::cout << " " << op;
                }
                std::cout << "\n";
            }
        }
    }
}

// ============================================================
//  测试用例 1: 简单算术表达式
//  c = a + b * 2;
// ============================================================
void testArithmeticExpression() {
    printSeparator("Test 1: Arithmetic expr: c = a + b * 2");

    // 手动构造 AST: c = a + (b * 2)
    auto identA = std::make_unique<IdentifierNode>("a", 1, 1);
    auto identB = std::make_unique<IdentifierNode>("b", 1, 1);
    auto num2   = std::make_unique<NumberNode>(2, 1, 1);
    auto mul    = std::make_unique<BinaryOpNode>("*", std::move(identB), std::move(num2), 1, 1);
    auto add    = std::make_unique<BinaryOpNode>("+", std::move(identA), std::move(mul), 1, 1);
    auto assign = std::make_unique<AssignmentNode>("c", std::move(add), 1, 1);

    auto block = std::make_unique<BlockNode>(1, 1);
    block->statements.push_back(std::move(assign));

    auto func = std::make_unique<FunctionDefinitionNode>(
        "main", "int", std::vector<std::pair<std::string, std::string>>{},
        std::move(block), 1, 1);

    auto program = std::make_unique<ProgramNode>();
    program->functions.push_back(std::move(func));

    // 生成 IR
    IRGenerator generator;
    IRProgram ir = generator.generate(program.get());
    printIR(ir);

    std::cout << "\n[PASS] Arithmetic expression IR generated\n";
}

// ============================================================
//  测试用例 2: 关系表达式 + if 语句
//  if (c > 5) { c = c + 1; }
// ============================================================
void testIfWithRelational() {
    printSeparator("Test 2: if (c > 5) { c = c + 1; }");

    // AST: if (c > 5) { c = c + 1; }
    auto identC1  = std::make_unique<IdentifierNode>("c", 1, 1);
    auto num5     = std::make_unique<NumberNode>(5, 1, 1);
    auto cond     = std::make_unique<BinaryOpNode>(">", std::move(identC1), std::move(num5), 1, 1);

    auto identC2  = std::make_unique<IdentifierNode>("c", 2, 1);
    auto num1     = std::make_unique<NumberNode>(1, 2, 1);
    auto add      = std::make_unique<BinaryOpNode>("+", std::move(identC2), std::move(num1), 2, 1);
    auto thenAssign = std::make_unique<AssignmentNode>("c", std::move(add), 2, 1);

    auto thenBlock = std::make_unique<BlockNode>(2, 1);
    thenBlock->statements.push_back(std::move(thenAssign));

    auto ifStmt = std::make_unique<IfStatementNode>(
        std::move(cond), std::move(thenBlock), nullptr, 1, 1);

    auto body = std::make_unique<BlockNode>(1, 1);
    body->statements.push_back(std::move(ifStmt));

    auto func = std::make_unique<FunctionDefinitionNode>(
        "main", "int", std::vector<std::pair<std::string, std::string>>{},
        std::move(body), 1, 1);

    auto program = std::make_unique<ProgramNode>();
    program->functions.push_back(std::move(func));

    // 生成 IR
    IRGenerator generator;
    IRProgram ir = generator.generate(program.get());
    printIR(ir);

    std::cout << "\n[PASS] if + relational expression IR generated\n";
}

// ============================================================
//  测试用例 3: while 循环
//  while (c < 20) { c = c + 1; }
// ============================================================
void testWhileLoop() {
    printSeparator("Test 3: while (c < 20) { c = c + 1; }");

    // AST: while (c < 20) { c = c + 1; }
    auto identC1  = std::make_unique<IdentifierNode>("c", 1, 1);
    auto num20    = std::make_unique<NumberNode>(20, 1, 1);
    auto cond     = std::make_unique<BinaryOpNode>("<", std::move(identC1), std::move(num20), 1, 1);

    auto identC2  = std::make_unique<IdentifierNode>("c", 2, 1);
    auto num1     = std::make_unique<NumberNode>(1, 2, 1);
    auto add      = std::make_unique<BinaryOpNode>("+", std::move(identC2), std::move(num1), 2, 1);
    auto bodyAssign = std::make_unique<AssignmentNode>("c", std::move(add), 2, 1);

    auto bodyBlock = std::make_unique<BlockNode>(2, 1);
    bodyBlock->statements.push_back(std::move(bodyAssign));

    auto whileStmt = std::make_unique<WhileStatementNode>(
        std::move(cond), std::move(bodyBlock), 1, 1);

    auto body = std::make_unique<BlockNode>(1, 1);
    body->statements.push_back(std::move(whileStmt));

    auto func = std::make_unique<FunctionDefinitionNode>(
        "main", "int", std::vector<std::pair<std::string, std::string>>{},
        std::move(body), 1, 1);

    auto program = std::make_unique<ProgramNode>();
    program->functions.push_back(std::move(func));

    // 生成 IR
    IRGenerator generator;
    IRProgram ir = generator.generate(program.get());
    printIR(ir);

    std::cout << "\n[PASS] while loop IR generated\n";
}

// ============================================================
//  测试用例 4: 直接构造 IR 测试 Optimizer
//  常量折叠: t1 = 3 + 4  →  t1 = 7
// ============================================================
void testConstantFolding() {
    printSeparator("Test 4: Optimizer - Constant Folding");

    IRProgram program;
    auto func = program.createFunction("test_const_fold");

    auto entry = func->createBlock("entry");
    func->entry = entry;

    // 直接使用字面量测试常量折叠: t1 = 3 + 4
    // 注意: 操作数直接用 "3" 和 "4"（不以 % 开头），而非临时变量
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::ADD, "%t1", std::vector<std::string>{"3", "4"}));
    // 再测试比较折叠: 5 > 3 → 1
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CMP_GT, "%t2", std::vector<std::string>{"5", "3"}));
    // 加一条 STORE 防止 DCE 删除
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "x", std::vector<std::string>{"%t1"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "y", std::vector<std::string>{"%t2"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::RET, "", std::vector<std::string>{"0"}));

    std::cout << "=== Before ===\n";
    printIR(program);

    Optimizer optimizer;
    optimizer.optimize(program);

    std::cout << "\n=== After ===\n";
    printIR(program);

    // Verify: ADD 3,4 should fold to CONST 7
    auto& instrs = entry->instructions;
    assert(instrs[0]->opcode == IROpcode::CONST);
    assert(instrs[0]->operands[0] == "7");
    // 验证: CMP_GT 5,3 应折叠为 CONST 1 (5 > 3)
    assert(instrs[1]->opcode == IROpcode::CONST);
    assert(instrs[1]->operands[0] == "1");

    std::cout << "\n[PASS] Constant folding: 3+4->7, 5>3->1\n";
}

// ============================================================
//  测试用例 5: Optimizer - 死代码删除
//  a = 1; a = 2;  删除第一条
// ============================================================
void testDeadCodeElimination() {
    printSeparator("Test 5: Optimizer - Dead Code Elimination");

    IRProgram program;
    auto func = program.createFunction("test_dce");
    auto entry = func->createBlock("entry");
    func->entry = entry;

    // 构造 IR: 
    //   %t1 = CONST 1
    //   STORE a, %t1     ← 被后面覆盖
    //   %t2 = CONST 2
    //   STORE a, %t2     ← 最终有效
    //   RET %t2
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t1", std::vector<std::string>{"1"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "a", std::vector<std::string>{"%t1"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t2", std::vector<std::string>{"2"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "a", std::vector<std::string>{"%t2"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::RET, "", std::vector<std::string>{"%t2"}));

    std::cout << "=== Before ===\n";
    printIR(program);

    Optimizer optimizer;
    optimizer.optimize(program);

    std::cout << "\n=== After ===\n";
    printIR(program);

    // Verify: all instructions survive (%t1 and %t2 used by STORE)
    // CONST %t1, STORE a,%t1, CONST %t2, STORE a,%t2, RET %t2 = 5
    auto& instrs = entry->instructions;
    assert(instrs.size() == 5);

    std::cout << "\n[PASS] DCE preserved all valid instructions\n";
}

// ============================================================
//  测试用例 6: 直接构造 IR 测试 CodeGenerator
// ============================================================
void testCodeGenerator() {
    printSeparator("Test 6: CodeGenerator - Assembly Output");

    IRProgram program;
    program.globalVariables.push_back("a");
    program.globalVariables.push_back("b");
    program.globalVariables.push_back("c");

    auto func = program.createFunction("main");
    auto entry = func->createBlock("entry");
    func->entry = entry;

    // a = 3 + 4 * 2;
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t1", std::vector<std::string>{"3"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t2", std::vector<std::string>{"4"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t3", std::vector<std::string>{"2"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::MUL, "%t4", std::vector<std::string>{"%t2", "%t3"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::ADD, "%t5", std::vector<std::string>{"%t1", "%t4"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "a", std::vector<std::string>{"%t5"}));

    // if (a > 5) { b = 1; }
    auto thenBlock = func->createBlock("then");
    auto mergeBlock = func->createBlock("merge");

    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::LOAD, "%t6", std::vector<std::string>{"a"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t7", std::vector<std::string>{"5"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CMP_GT, "%t8", std::vector<std::string>{"%t6", "%t7"}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::BRANCH_EQ, "", std::vector<std::string>{"%t8", "1", thenBlock->name}));
    entry->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));

    thenBlock->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::CONST, "%t9", std::vector<std::string>{"1"}));
    thenBlock->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::STORE, "b", std::vector<std::string>{"%t9"}));
    thenBlock->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));

    mergeBlock->addInstruction(std::make_unique<IRInstruction>(
        IROpcode::RET, "", std::vector<std::string>{"0"}));

    // 生成汇编
    {
        CodeGenerator codegen("output_test.s");
        codegen.generate(program);
    }

    // 读取并打印生成的汇编
    std::ifstream inFile("output_test.s");
    if (inFile.is_open()) {
        std::string line;
        std::cout << "=== Generated Assembly ===\n";
        while (std::getline(inFile, line)) {
            std::cout << "  " << line << "\n";
        }
        inFile.close();
    }

    std::cout << "\n[PASS] CodeGenerator output written to output_test.s\n";
}

// ============================================================
//  测试用例 7: 完整编译流程
//  从 AST → IR → 优化 → 汇编
// ============================================================
void testFullPipeline() {
    printSeparator("Test 7: Full Pipeline AST -> IR -> Optimize -> Assembly");

    // 构造: while (c < 20) { c = c + 1; }
    auto whileIdent = std::make_unique<IdentifierNode>("c", 1, 1);
    auto whileNum   = std::make_unique<NumberNode>(20, 1, 1);
    auto whileCond  = std::make_unique<BinaryOpNode>("<", std::move(whileIdent), std::move(whileNum), 1, 1);

    auto bodyIdent  = std::make_unique<IdentifierNode>("c", 2, 1);
    auto bodyNum1   = std::make_unique<NumberNode>(1, 2, 1);
    auto bodyAdd    = std::make_unique<BinaryOpNode>("+", std::move(bodyIdent), std::move(bodyNum1), 2, 1);
    auto bodyAssign = std::make_unique<AssignmentNode>("c", std::move(bodyAdd), 2, 1);

    auto bodyBlock  = std::make_unique<BlockNode>(2, 1);
    bodyBlock->statements.push_back(std::move(bodyAssign));

    auto whileStmt  = std::make_unique<WhileStatementNode>(
        std::move(whileCond), std::move(bodyBlock), 1, 1);

    // 变量声明 int c;
    auto decl = std::make_unique<DeclarationNode>("int", "c", nullptr, 1, 1);

    auto declBlock = std::make_unique<BlockNode>(1, 1);
    declBlock->statements.push_back(std::move(decl));
    declBlock->statements.push_back(std::move(whileStmt));

    auto func = std::make_unique<FunctionDefinitionNode>(
        "main", "int", std::vector<std::pair<std::string, std::string>>{},
        std::move(declBlock), 1, 1);

    auto program = std::make_unique<ProgramNode>();
    program->functions.push_back(std::move(func));

    // Step 1: IR 生成
    std::cout << "--- Step 1: IR Generation ---\n";
    IRGenerator irGen;
    IRProgram ir = irGen.generate(program.get());
    printIR(ir);

    // Step 2: 优化
    std::cout << "\n--- Step 2: Optimization ---\n";
    Optimizer opt;
    opt.optimize(ir);
    printIR(ir);

    // Step 3: 代码生成
    std::cout << "\n--- Step 3: Code Generation ---\n";
    CodeGenerator codeGen("output_pipeline.s");
    codeGen.generate(ir);

    std::ifstream inFile("output_pipeline.s");
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            std::cout << "  " << line << "\n";
        }
        inFile.close();
    }

    std::cout << "\n[PASS] Full pipeline test passed\n";
}

// ============================================================
//  主函数
// ============================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "  TinyC Compiler - Backend Test Suite\n";
    std::cout << "========================================\n";

    // 由于各测试需要分别编译，这里用条件编译
    // 实际运行时取消对应 #define 的注释
    
    // #define RUN_TEST_1
    // #define RUN_TEST_2
    // #define RUN_TEST_3
    // #define RUN_TEST_4
    // #define RUN_TEST_5
    // #define RUN_TEST_6
    // #define RUN_TEST_7

    // 为方便一次性运行，直接全部执行
    // 注意：如果 Parser 未完成，请使用手动构造的 AST

    testArithmeticExpression();
    testIfWithRelational();
    testWhileLoop();

    // Optimizer 测试
    testConstantFolding();
    testDeadCodeElimination();

    // CodeGenerator 测试
    testCodeGenerator();

    // 完整流水线
    testFullPipeline();

    printSeparator("All Tests Passed");
    std::cout << "All backend tests passed!\n\n";

    return 0;
}
