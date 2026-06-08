#include "../include/AST.h"
#include "../include/Symbol.h"
#include <stdexcept>

class SemanticAnalyzer {
private:
    SymbolTable symbolTable;

    void visitProgramNode(ProgramNode* node);
    void visitDeclarationNode(DeclarationNode* node);
    void visitFunctionDefinitionNode(FunctionDefinitionNode* node);
    void visitStatementNode(ASTNode* node);
    void visitExpressionNode(ASTNode* node);
    SymbolType visitExpressionAndGetType(ASTNode* node);

public:
    void analyze(ASTNode* root) {
        if (!root) return;
        visitProgramNode(dynamic_cast<ProgramNode*>(root));
    }
};

void SemanticAnalyzer::visitProgramNode(ProgramNode* node) {
    for (auto& decl : node->declarations) {
        visitDeclarationNode(dynamic_cast<DeclarationNode*>(decl.get()));
    }
    for (auto& func : node->functions) {
        visitFunctionDefinitionNode(dynamic_cast<FunctionDefinitionNode*>(func.get()));
    }
}

void SemanticAnalyzer::visitDeclarationNode(DeclarationNode* node) {
    SymbolType type;
    if (node->type == "int") type = SymbolType::INTEGER;
    else if (node->type == "float") type = SymbolType::FLOAT;
    else if (node->type == "void") type = SymbolType::VOID;
    else throw std::runtime_error("Unknown type: " + node->type);

    if (symbolTable.existsInCurrentScope(node->name)) {
        throw std::runtime_error("Variable " + node->name + " already declared in this scope");
    }

    Symbol* symbol = new Symbol(node->name, type, symbolTable.getCurrentScope(), node->line);
    symbolTable.insert(symbol);

    if (node->initializer) {
        SymbolType exprType = visitExpressionAndGetType(node->initializer.get());
        if (type != exprType) {
            throw std::runtime_error("Type mismatch in declaration");
        }
    }
}

void SemanticAnalyzer::visitFunctionDefinitionNode(FunctionDefinitionNode* node) {
    SymbolType returnType;
    if (node->returnType == "int") returnType = SymbolType::INTEGER;
    else if (node->returnType == "float") returnType = SymbolType::FLOAT;
    else if (node->returnType == "void") returnType = SymbolType::VOID;
    else throw std::runtime_error("Unknown return type: " + node->returnType);

    std::vector<SymbolType> paramTypes;
    for (auto& param : node->parameters) {
        if (param.second == "int") paramTypes.push_back(SymbolType::INTEGER);
        else if (param.second == "float") paramTypes.push_back(SymbolType::FLOAT);
        else throw std::runtime_error("Unknown parameter type: " + param.second);
    }

    FunctionSymbol* funcSymbol = new FunctionSymbol(node->name, returnType, paramTypes, 
                                                    symbolTable.getCurrentScope(), node->line);
    symbolTable.insert(funcSymbol);

    symbolTable.enterScope();

    for (auto& param : node->parameters) {
        SymbolType paramType;
        if (param.second == "int") paramType = SymbolType::INTEGER;
        else paramType = SymbolType::FLOAT;
        Symbol* paramSymbol = new Symbol(param.first, paramType, symbolTable.getCurrentScope(), node->line);
        symbolTable.insert(paramSymbol);
    }

    visitStatementNode(node->body.get());

    symbolTable.exitScope();
}

void SemanticAnalyzer::visitStatementNode(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::BLOCK: {
            symbolTable.enterScope();
            auto block = dynamic_cast<BlockNode*>(node);
            for (auto& stmt : block->statements) {
                visitStatementNode(stmt.get());
            }
            symbolTable.exitScope();
            break;
        }
        case ASTNodeType::IF_STATEMENT: {
            auto ifStmt = dynamic_cast<IfStatementNode*>(node);
            visitExpressionNode(ifStmt->condition.get());
            visitStatementNode(ifStmt->thenBranch.get());
            if (ifStmt->elseBranch) {
                visitStatementNode(ifStmt->elseBranch.get());
            }
            break;
        }
        case ASTNodeType::WHILE_STATEMENT: {
            auto whileStmt = dynamic_cast<WhileStatementNode*>(node);
            visitExpressionNode(whileStmt->condition.get());
            visitStatementNode(whileStmt->body.get());
            break;
        }
        case ASTNodeType::FOR_STATEMENT: {
            auto forStmt = dynamic_cast<ForStatementNode*>(node);
            if (forStmt->init) visitStatementNode(forStmt->init.get());
            if (forStmt->condition) visitExpressionNode(forStmt->condition.get());
            if (forStmt->increment) visitExpressionNode(forStmt->increment.get());
            visitStatementNode(forStmt->body.get());
            break;
        }
        case ASTNodeType::RETURN_STATEMENT: {
            auto returnStmt = dynamic_cast<ReturnStatementNode*>(node);
            if (returnStmt->value) {
                visitExpressionNode(returnStmt->value.get());
            }
            break;
        }
        case ASTNodeType::ASSIGNMENT: {
            auto assign = dynamic_cast<AssignmentNode*>(node);
            Symbol* symbol = symbolTable.lookup(assign->name);
            if (!symbol) {
                throw std::runtime_error("Undefined variable: " + assign->name);
            }
            visitExpressionNode(assign->value.get());
            break;
        }
        case ASTNodeType::DECLARATION: {
            visitDeclarationNode(dynamic_cast<DeclarationNode*>(node));
            break;
        }
        default:
            visitExpressionNode(node);
    }
}

void SemanticAnalyzer::visitExpressionNode(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::BINARY_OP: {
            auto binOp = dynamic_cast<BinaryOpNode*>(node);
            visitExpressionNode(binOp->left.get());
            visitExpressionNode(binOp->right.get());
            break;
        }
        case ASTNodeType::UNARY_OP: {
            auto unOp = dynamic_cast<UnaryOpNode*>(node);
            visitExpressionNode(unOp->operand.get());
            break;
        }
        case ASTNodeType::IDENTIFIER: {
            auto ident = dynamic_cast<IdentifierNode*>(node);
            Symbol* symbol = symbolTable.lookup(ident->name);
            if (!symbol) {
                throw std::runtime_error("Undefined variable: " + ident->name);
            }
            break;
        }
        case ASTNodeType::NUMBER:
            break;
        default:
            break;
    }
}

SymbolType SemanticAnalyzer::visitExpressionAndGetType(ASTNode* node) {
    if (!node) return SymbolType::VOID;

    switch (node->type) {
        case ASTNodeType::IDENTIFIER: {
            auto ident = dynamic_cast<IdentifierNode*>(node);
            Symbol* symbol = symbolTable.lookup(ident->name);
            if (!symbol) {
                throw std::runtime_error("Undefined variable: " + ident->name);
            }
            return symbol->type;
        }
        case ASTNodeType::NUMBER: {
            auto num = dynamic_cast<NumberNode*>(node);
            return num->isFloat ? SymbolType::FLOAT : SymbolType::INTEGER;
        }
        case ASTNodeType::BINARY_OP: {
            auto binOp = dynamic_cast<BinaryOpNode*>(node);
            SymbolType leftType = visitExpressionAndGetType(binOp->left.get());
            SymbolType rightType = visitExpressionAndGetType(binOp->right.get());
            if (leftType != rightType) {
                throw std::runtime_error("Type mismatch in binary operation");
            }
            return leftType;
        }
        case ASTNodeType::UNARY_OP: {
            auto unOp = dynamic_cast<UnaryOpNode*>(node);
            return visitExpressionAndGetType(unOp->operand.get());
        }
        default:
            return SymbolType::VOID;
    }
}
