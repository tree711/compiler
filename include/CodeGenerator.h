#pragma once

#include "IR.h"
#include <fstream>
#include <unordered_map>

class CodeGenerator {
private:
    std::ofstream output;
    std::unordered_map<std::string, int> variableOffsets;
    int stackOffset;

    void generateFunction(FunctionIR& func);
    void generateBasicBlock(BasicBlock& block);
    std::string getOperand(const std::string& op);

public:
    CodeGenerator(const std::string& filename);
    ~CodeGenerator();
    void generate(IRProgram& program);
};
