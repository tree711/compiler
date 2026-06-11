#include "../include/IRGenerator.h"
#include <sstream>
#include <algorithm>

static void addGlobalVariable(IRProgram& program, const std::string& name) {
    if (std::find(program.globalVariables.begin(), program.globalVariables.end(), name) ==
        program.globalVariables.end()) {
        program.globalVariables.push_back(name);
    }
}

std::string IRGenerator::newTemp() {
    std::stringstream ss;
    ss << "%t" << tempCounter++;
    return ss.str();
}

std::string IRGenerator::newLabel(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "_" << labelCounter++;
    return ss.str();
}

IRProgram IRGenerator::generate(ASTNode* root) {
    if (auto programNode = dynamic_cast<ProgramNode*>(root)) {
        // 情况1：有函数定义
        if (!programNode->functions.empty()) {
            for (auto& func : programNode->functions) {
                generateFunctionDefinition(dynamic_cast<FunctionDefinitionNode*>(func.get()));
            }
        }
        // 情况2：没有函数定义，只有全局语句（TinyC 风格）
        else if (!programNode->children.empty()) {
            // 创建默认的 main 函数
            currentFunction = program.createFunction("main");
            currentBlock = currentFunction->createBlock("entry");
            currentFunction->entry = currentBlock;
            
            // 处理所有全局语句
            for (auto& child : programNode->children) {
                generateStatement(child.get());
            }
            
            // 添加默认返回
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::RET, "", std::vector<std::string>{"0"}));
        }
        // 情况3：有全局声明
        else if (!programNode->declarations.empty()) {
            // 创建默认的 main 函数
            currentFunction = program.createFunction("main");
            currentBlock = currentFunction->createBlock("entry");
            currentFunction->entry = currentBlock;
            
            // 处理全局声明
            for (auto& decl : programNode->declarations) {
                generateStatement(decl.get());
            }
            
            // 添加默认返回
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::RET, "", std::vector<std::string>{"0"}));
        }
    }
    return std::move(program);
}

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
            std::string varName;
            std::string value;
            
            // 支持两种 AssignmentNode 构造方式
            if (!assign->name.empty()) {
                // 新方式：name + value
                varName = assign->name;
                value = generateExpression(assign->value.get());
            } else if (assign->left) {
                // 旧方式：left + right
                auto ident = dynamic_cast<IdentifierNode*>(assign->left.get());
                if (ident) {
                    varName = ident->name;
                }
                value = generateExpression(assign->right.get());
            }
            
            if (!varName.empty()) {
                addGlobalVariable(program, varName);
                currentBlock->addInstruction(std::make_unique<IRInstruction>(
                    IROpcode::STORE, varName, std::vector<std::string>{value}));
            }
            break;
        }
        case ASTNodeType::IF_STATEMENT: {
            auto ifStmt = dynamic_cast<IfStatementNode*>(node);
            std::string cond = generateExpression(ifStmt->condition.get());
            
            BasicBlock* thenBlock = currentFunction->createBlock(newLabel("then"));
            BasicBlock* elseBlock = currentFunction->createBlock(newLabel("else"));

            // 条件值为 1 时跳转到真分支，否则跳转到假分支
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH_EQ, "", std::vector<std::string>{cond, "1", thenBlock->name}));
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{elseBlock->name}));

            currentBlock = thenBlock;
            generateStatement(ifStmt->thenBranch.get());
            BasicBlock* thenEnd = currentBlock;

            currentBlock = elseBlock;
            if (ifStmt->elseBranch) {
                generateStatement(ifStmt->elseBranch.get());
            }
            BasicBlock* elseEnd = currentBlock;

            // 在嵌套分支生成完成后再创建汇合块，
            // 避免线性输出汇编时从外层汇合块错误落入内层分支
            BasicBlock* mergeBlock = currentFunction->createBlock(newLabel("merge"));
            thenEnd->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));
            elseEnd->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{mergeBlock->name}));

            currentBlock = mergeBlock;
            break;
        }
        case ASTNodeType::WHILE_STATEMENT: {
            auto whileStmt = dynamic_cast<WhileStatementNode*>(node);
            BasicBlock* condBlock = currentFunction->createBlock(newLabel("while_cond"));
            BasicBlock* bodyBlock = currentFunction->createBlock(newLabel("while_body"));

            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{condBlock->name}));

            currentBlock = condBlock;
            std::string cond = generateExpression(whileStmt->condition.get());
            std::string exitLabel = newLabel("while_exit");
            // 条件值为 1 时进入循环体，否则跳转到循环出口
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH_EQ, "", std::vector<std::string>{cond, "1", bodyBlock->name}));
            currentBlock->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{exitLabel}));

            currentBlock = bodyBlock;
            generateStatement(whileStmt->body.get());
            BasicBlock* bodyEnd = currentBlock;
            bodyEnd->addInstruction(std::make_unique<IRInstruction>(
                IROpcode::BRANCH, "", std::vector<std::string>{condBlock->name}));

            BasicBlock* exitBlock = currentFunction->createBlock(exitLabel);
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
                    IROpcode::RET, "", std::vector<std::string>{}));
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
            
            // 支持多变量声明
            if (!decl->variables.empty()) {
                for (auto& var : decl->variables) {
                    addGlobalVariable(program, var->name);
                }
            }
            // 支持单变量声明
            else if (!decl->name.empty()) {
                addGlobalVariable(program, decl->name);
                if (decl->initializer) {
                    std::string value = generateExpression(decl->initializer.get());
                    currentBlock->addInstruction(std::make_unique<IRInstruction>(
                        IROpcode::STORE, decl->name, std::vector<std::string>{value}));
                }
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
            else if (binOp->op == "==") opcode = IROpcode::CMP_EQ;
            else if (binOp->op == "!=") opcode = IROpcode::CMP_NE;
            else if (binOp->op == "<")  opcode = IROpcode::CMP_LT;
            else if (binOp->op == ">")  opcode = IROpcode::CMP_GT;
            else if (binOp->op == "<=") opcode = IROpcode::CMP_LE;
            else if (binOp->op == ">=") opcode = IROpcode::CMP_GE;
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
