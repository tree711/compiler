#pragma once

#include "AST.h"
#include "Symbol.h"

class SemanticAnalyzer {
private:
    SymbolTable symbolTable;

    void visitProgram(ProgramNode* node);
    void visitDeclaration(DeclarationNode* node);
    void visitStatement(ASTNode* node);
    SymbolType visitExpression(ASTNode* node);
    void requireDeclared(const std::string& name, int line, int column) const;

public:
    void analyze(ASTNode* root);
};
