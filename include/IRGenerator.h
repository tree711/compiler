#pragma once

#include "AST.h"
#include "IR.h"

class IRGenerator {
private:
    IRProgram program;
    FunctionIR* currentFunction;
    BasicBlock* currentBlock;
    int tempCounter;
    int labelCounter;

    std::string newTemp();
    std::string newLabel(const std::string& prefix);
    std::string generateExpression(ASTNode* node);
    void generateStatement(ASTNode* node);
    void generateFunctionDefinition(FunctionDefinitionNode* node);

public:
    IRGenerator() : currentFunction(nullptr), currentBlock(nullptr), tempCounter(0), labelCounter(0) {}
    IRProgram generate(ASTNode* root);
};
