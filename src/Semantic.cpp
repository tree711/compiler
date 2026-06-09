#include "../include/Semantic.h"

#include <stdexcept>
#include <string>

namespace {

std::string location(int line, int column) {
    return " at line " + std::to_string(line) +
           ", column " + std::to_string(column);
}

class ScopeGuard {
private:
    SymbolTable& table;

public:
    explicit ScopeGuard(SymbolTable& table) : table(table) {
        table.enterScope();
    }

    ~ScopeGuard() {
        table.exitScope();
    }
};

}  // namespace

SymbolTable::SymbolTable() : scopes(1) {}

void SymbolTable::enterScope() {
    scopes.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes.size() <= 1) {
        throw std::logic_error("Cannot exit global scope");
    }
    scopes.pop_back();
}

bool SymbolTable::declare(
    const std::string& name, SymbolType type, int line, int column) {
    // The backend currently emits variables by source name, so shadowing
    // would map distinct declarations to the same assembly symbol.
    if (lookup(name)) {
        return false;
    }

    const int scope = currentScope();
    scopes.back().emplace(
        name, Symbol{name, type, scope, line, column});
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        const auto symbol = scope->find(name);
        if (symbol != scope->end()) {
            return &symbol->second;
        }
    }
    return nullptr;
}

bool SymbolTable::existsInCurrentScope(const std::string& name) const {
    return scopes.back().find(name) != scopes.back().end();
}

int SymbolTable::currentScope() const {
    return static_cast<int>(scopes.size()) - 1;
}

void SemanticAnalyzer::analyze(ASTNode* root) {
    auto* program = dynamic_cast<ProgramNode*>(root);
    if (!program) {
        throw std::runtime_error("Semantic analysis requires a ProgramNode");
    }
    visitProgram(program);
}

void SemanticAnalyzer::visitProgram(ProgramNode* node) {
    for (auto& declaration : node->declarations) {
        auto* declarationNode =
            dynamic_cast<DeclarationNode*>(declaration.get());
        if (!declarationNode) {
            throw std::runtime_error(
                "Program declaration list contains a non-declaration node");
        }
        visitDeclaration(declarationNode);
    }

    for (auto& statement : node->statements) {
        visitStatement(statement.get());
    }
}

void SemanticAnalyzer::visitDeclaration(DeclarationNode* node) {
    if (node->type != "int") {
        throw std::runtime_error(
            "Unsupported type '" + node->type + "'" +
            location(node->line, node->column));
    }

    if (node->initializer) {
        visitExpression(node->initializer.get());
    }

    if (!symbolTable.declare(
            node->name, SymbolType::INTEGER, node->line, node->column)) {
        throw std::runtime_error(
            "Variable '" + node->name + "' is already declared" +
            location(node->line, node->column));
    }
}

void SemanticAnalyzer::visitStatement(ASTNode* node) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case ASTNodeType::DECLARATION:
            visitDeclaration(dynamic_cast<DeclarationNode*>(node));
            return;
        case ASTNodeType::ASSIGNMENT:
            visitExpression(node);
            return;
        case ASTNodeType::BLOCK: {
            ScopeGuard scope(symbolTable);
            auto* block = dynamic_cast<BlockNode*>(node);
            for (auto& statement : block->statements) {
                visitStatement(statement.get());
            }
            return;
        }
        case ASTNodeType::IF_STATEMENT: {
            auto* ifStatement = dynamic_cast<IfStatementNode*>(node);
            visitExpression(ifStatement->condition.get());
            visitStatement(ifStatement->thenBranch.get());
            visitStatement(ifStatement->elseBranch.get());
            return;
        }
        case ASTNodeType::WHILE_STATEMENT: {
            auto* whileStatement = dynamic_cast<WhileStatementNode*>(node);
            visitExpression(whileStatement->condition.get());
            visitStatement(whileStatement->body.get());
            return;
        }
        case ASTNodeType::BINARY_OP:
        case ASTNodeType::UNARY_OP:
        case ASTNodeType::IDENTIFIER:
        case ASTNodeType::NUMBER:
            visitExpression(node);
            return;
        case ASTNodeType::PROGRAM:
            throw std::runtime_error("Nested ProgramNode is not valid");
    }

    throw std::runtime_error("Unsupported statement node");
}

SymbolType SemanticAnalyzer::visitExpression(ASTNode* node) {
    if (!node) {
        throw std::runtime_error("Missing expression");
    }

    switch (node->type) {
        case ASTNodeType::NUMBER:
            return SymbolType::INTEGER;
        case ASTNodeType::IDENTIFIER: {
            auto* identifier = dynamic_cast<IdentifierNode*>(node);
            requireDeclared(
                identifier->name, identifier->line, identifier->column);
            return SymbolType::INTEGER;
        }
        case ASTNodeType::ASSIGNMENT: {
            auto* assignment = dynamic_cast<AssignmentNode*>(node);
            requireDeclared(
                assignment->name, assignment->line, assignment->column);
            visitExpression(assignment->value.get());
            return SymbolType::INTEGER;
        }
        case ASTNodeType::UNARY_OP: {
            auto* unary = dynamic_cast<UnaryOpNode*>(node);
            if (unary->op != "-") {
                throw std::runtime_error(
                    "Unsupported unary operator '" + unary->op + "'" +
                    location(unary->line, unary->column));
            }
            visitExpression(unary->operand.get());
            return SymbolType::INTEGER;
        }
        case ASTNodeType::BINARY_OP: {
            auto* binary = dynamic_cast<BinaryOpNode*>(node);
            const std::string& op = binary->op;
            if (op != "+" && op != "-" && op != "*" && op != "/" &&
                op != "==" && op != "<" && op != ">") {
                throw std::runtime_error(
                    "Unsupported binary operator '" + op + "'" +
                    location(binary->line, binary->column));
            }
            visitExpression(binary->left.get());
            visitExpression(binary->right.get());
            return SymbolType::INTEGER;
        }
        default:
            throw std::runtime_error(
                "Node is not a valid expression" +
                location(node->line, node->column));
    }
}

void SemanticAnalyzer::requireDeclared(
    const std::string& name, int line, int column) const {
    if (!symbolTable.lookup(name)) {
        throw std::runtime_error(
            "Variable '" + name + "' is not declared" +
            location(line, column));
    }
}
