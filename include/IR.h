#pragma once

#include <string>
#include <vector>
#include <memory>

enum class IROpcode {
    ADD, SUB, MUL, DIV,
    LOAD, STORE,
    CONST,
    BRANCH, BRANCH_EQ, BRANCH_NE, BRANCH_LT, BRANCH_GT, BRANCH_LE, BRANCH_GE,
    CALL, RET,
    PHI,
    // 比较指令（结果为 0 或 1）
    CMP_EQ, CMP_NE, CMP_LT, CMP_GT, CMP_LE, CMP_GE
};

class IRInstruction {
public:
    IROpcode opcode;
    std::string result;
    std::vector<std::string> operands;
    std::string label;

    IRInstruction(IROpcode op, const std::string& res, const std::vector<std::string>& ops, const std::string& lbl = "")
        : opcode(op), result(res), operands(ops), label(lbl) {}
};

using IRInstructionPtr = std::unique_ptr<IRInstruction>;

class BasicBlock {
public:
    std::string name;
    std::vector<IRInstructionPtr> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;

    BasicBlock(const std::string& n) : name(n) {}

    void addInstruction(IRInstructionPtr instr) {
        instructions.push_back(std::move(instr));
    }

    void addSuccessor(BasicBlock* block) {
        successors.push_back(block);
        block->predecessors.push_back(this);
    }
};

class FunctionIR {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entry;

    FunctionIR(const std::string& n) : name(n), entry(nullptr) {}

    BasicBlock* createBlock(const std::string& name) {
        blocks.push_back(std::make_unique<BasicBlock>(name));
        return blocks.back().get();
    }
};

class IRProgram {
public:
    std::vector<std::unique_ptr<FunctionIR>> functions;
    std::vector<std::string> globalVariables;

    FunctionIR* createFunction(const std::string& name) {
        functions.push_back(std::make_unique<FunctionIR>(name));
        return functions.back().get();
    }
};
