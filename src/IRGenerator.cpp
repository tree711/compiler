#include "../include/AST.h"
#include "../include/IR.h"
#include <memory>
#include <sstream>

class IRGenerator {
private:
    IRProgram program;
    FunctionIR* currentFunction;
    BasicBlock* currentBlock;
    int tempCounter;

    std::string newTemp() {
        std::stringstream ss;
        ss << "%t" << tempCounter++;
        return ss.str();
    }

    std::string generateExpression(ASTNode* node);
    void generateStatement(ASTNode* node);
    void generateFunctionDefinition(FunctionDefinitionNode* node);

public:
    IRGenerator() : currentFunction(nullptr), currentBlock(nullptr), tempCounter(0) {}

    IRProgram generate(ASTNode* root) {
        if (auto programNode = dynamic_cast<ProgramNode*>(root)) {
            for (auto& func : programNode->functions) {
                generateFunctionDefinition(dynamic_cast<FunctionDefinitionNode*>(func.get()));
            }
        }
        return std::move(program);
    }
};

void IRGenerator::generateFunctionDefinition(FunctionDefinitionNode* node) {
    currentFunction = program.createFunction(node->name);
    currentBlock = currentFunction->createBlock("entry");
    currentFunction->entry = currentBlock;

    std::vector<std::string> params;
    for (size_t i = 0; i < node->parameters.size(); i++) {
        std::stringstream ss;
        ss << "%p" << i;
        params.push_back(ss.str());
    }
    currentFunction->parameters = params;

    if (auto block = dynamic_cast<BlockNode*>(node->body.get())) {
        for (auto& stmt : block->statements) {
            generateStatement(stmt.get());
        }
    }
}

void IRGenerator::generateStatement(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::ASSIGNMENT: {
            auto assign = dynamic_cast<AssignmentNode*>(node);
            std::string value = generateExpression(assign->value.get());
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::STORE, assign->name, std::vector<std::string>{value}));
            break;
        }
        case ASTNodeType::IF_STATEMENT: {
            auto ifStmt = dynamic_cast<IfStatementNode*>(node);
            std::string cond = generateExpression(ifStmt->condition.get());
            
            BasicBlock* thenBlock = currentFunction->createBlock("then");
            BasicBlock* elseBlock = currentFunction->createBlock("else");
            BasicBlock* mergeBlock = currentFunction->createBlock("merge");

            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH_EQ, "", std::vector<std::string>{cond, "1", thenBlock->name, elseBlock->name}));

            currentBlock = thenBlock;
            generateStatement(ifStmt->thenBranch.get());
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));

            currentBlock = elseBlock;
            if (ifStmt->elseBranch) {
                generateStatement(ifStmt->elseBranch.get());
            }
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));

            currentBlock = mergeBlock;
            break;
        }
        case ASTNodeType::WHILE_STATEMENT: {
            auto whileStmt = dynamic_cast<WhileStatementNode*>(node);
            BasicBlock* condBlock = currentFunction->createBlock("while_cond");
            BasicBlock* bodyBlock = currentFunction->createBlock("while_body");
            BasicBlock* exitBlock = currentFunction->createBlock("while_exit");

            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{condBlock->name}));

            currentBlock = condBlock;
            std::string cond = generateExpression(whileStmt->condition.get());
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH_EQ, "", std::vector<std::string>{cond, "1", bodyBlock->name, exitBlock->name}));

            currentBlock = bodyBlock;
            generateStatement(whileStmt->body.get());
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{condBlock->name}));

            currentBlock = exitBlock;
            break;
        }
        case ASTNodeType::RETURN_STATEMENT: {
            auto returnStmt = dynamic_cast<ReturnStatementNode*>(node);
            if (returnStmt->value) {
                std::string value = generateExpression(returnStmt->value.get());
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::RET, "", std::vector<std::string>{value}));
            } else {
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::RET, "", {}));
            }
            break;
        }
        case ASTNodeType::BLOCK: {
            auto block = dynamic_cast<BlockNode*>(node);
            for (auto& stmt : block->statements) {
                generateStatement(stmt.get());
            }
            break;
        }
        case ASTNodeType::DECLARATION: {
            auto decl = dynamic_cast<DeclarationNode*>(node);
            if (decl->initializer) {
                std::string value = generateExpression(decl->initializer.get());
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::STORE, decl->name, std::vector<std::string>{value}));
            }
            break;
        }
        default:
            break;
    }
}

std::string IRGenerator::generateExpression(ASTNode* node) {
    if (!node) return "";

    switch (node->type) {
        case ASTNodeType::IDENTIFIER: {
            auto ident = dynamic_cast<IdentifierNode*>(node);
            std::string temp = newTemp();
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::LOAD, temp, std::vector<std::string>{ident->name}));
            return temp;
        }
        case ASTNodeType::NUMBER: {
            auto num = dynamic_cast<NumberNode*>(node);
            std::string temp = newTemp();
            std::string value = num->isFloat ? 
                std::to_string(std::get<double>(num->value)) : 
                std::to_string(std::get<int>(num->value));
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::CONST, temp, std::vector<std::string>{value}));
            return temp;
        }
        case ASTNodeType::BINARY_OP: {
            auto binOp = dynamic_cast<BinaryOpNode*>(node);
            std::string left = generateExpression(binOp->left.get());
            std::string right = generateExpression(binOp->right.get());
            std::string temp = newTemp();

            IROpcode opcode;
            if (binOp->op == "+") opcode = IROpcode::ADD;
            else if (binOp->op == "-") opcode = IROpcode::SUB;
            else if (binOp->op == "*") opcode = IROpcode::MUL;
            else if (binOp->op == "/") opcode = IROpcode::DIV;
            else return "";

            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                opcode, temp, std::vector<std::string>{left, right}));
            return temp;
        }
        case ASTNodeType::UNARY_OP: {
            auto unOp = dynamic_cast<UnaryOpNode*>(node);
            std::string operand = generateExpression(unOp->operand.get());
            std::string temp = newTemp();

            if (unOp->op == "-") {
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::CONST, temp, std::vector<std::string>{"0"}));
                std::string result = newTemp();
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::SUB, result, std::vector<std::string>{temp, operand}));
                return result;
            }
            return operand;
        }
        default:
            return "";
    }
}
