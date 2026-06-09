#pragma once

#include <memory>
#include <vector>
#include <string>

enum class ASTNodeType {
    PROGRAM,
    DECLARATION,
    ASSIGNMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    BLOCK,
    BINARY_OP,
    UNARY_OP,
    IDENTIFIER,
    NUMBER
};

class ASTNode {
public:
    ASTNodeType type;
    int line;
    int column;

    ASTNode(ASTNodeType t, int l, int c) : type(t), line(l), column(c) {}
    virtual ~ASTNode() = default;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class NumberNode : public ASTNode {
public:
    int value;

    NumberNode(int val, int l, int c)
        : ASTNode(ASTNodeType::NUMBER, l, c), value(val) {}
};

class IdentifierNode : public ASTNode {
public:
    std::string name;

    IdentifierNode(const std::string& n, int l, int c)
        : ASTNode(ASTNodeType::IDENTIFIER, l, c), name(n) {}
};

class BinaryOpNode : public ASTNode {
public:
    std::string op;
    ASTNodePtr left;
    ASTNodePtr right;

    BinaryOpNode(const std::string& o, ASTNodePtr l, ASTNodePtr r, int line, int col)
        : ASTNode(ASTNodeType::BINARY_OP, line, col),
          op(o), left(std::move(l)), right(std::move(r)) {}
};

class UnaryOpNode : public ASTNode {
public:
    std::string op;
    ASTNodePtr operand;

    UnaryOpNode(const std::string& o, ASTNodePtr opnd, int line, int col)
        : ASTNode(ASTNodeType::UNARY_OP, line, col),
          op(o), operand(std::move(opnd)) {}
};

class AssignmentNode : public ASTNode {
public:
    std::string name;
    ASTNodePtr value;

    AssignmentNode(const std::string& n, ASTNodePtr v, int line, int col)
        : ASTNode(ASTNodeType::ASSIGNMENT, line, col),
          name(n), value(std::move(v)) {}
};

class BlockNode : public ASTNode {
public:
    std::vector<ASTNodePtr> statements;

    BlockNode(int l, int c) : ASTNode(ASTNodeType::BLOCK, l, c) {}

    void addStatement(ASTNodePtr stmt) {
        statements.push_back(std::move(stmt));
    }
};

class IfStatementNode : public ASTNode {
public:
    ASTNodePtr condition;
    ASTNodePtr thenBranch;
    ASTNodePtr elseBranch;

    IfStatementNode(ASTNodePtr cond, ASTNodePtr thenB, ASTNodePtr elseB, int line, int col)
        : ASTNode(ASTNodeType::IF_STATEMENT, line, col),
          condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}
};

class WhileStatementNode : public ASTNode {
public:
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStatementNode(ASTNodePtr cond, ASTNodePtr b, int line, int col)
        : ASTNode(ASTNodeType::WHILE_STATEMENT, line, col),
          condition(std::move(cond)), body(std::move(b)) {}
};

class DeclarationNode : public ASTNode {
public:
    std::string type;
    std::string name;
    ASTNodePtr initializer;

    DeclarationNode(const std::string& t, const std::string& n, ASTNodePtr init, int line, int col)
        : ASTNode(ASTNodeType::DECLARATION, line, col),
          type(t), name(n), initializer(std::move(init)) {}
};

class ProgramNode : public ASTNode {
public:
    std::vector<ASTNodePtr> declarations;
    std::vector<ASTNodePtr> statements;

    ProgramNode() : ASTNode(ASTNodeType::PROGRAM, 0, 0) {}

    void addDeclaration(ASTNodePtr decl) {
        declarations.push_back(std::move(decl));
    }

    void addStatement(ASTNodePtr stmt) {
        statements.push_back(std::move(stmt));
    }
};
