#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

enum class ASTNodeType {
    PROGRAM,
    DECLARATION,
    ASSIGNMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    FOR_STATEMENT,
    RETURN_STATEMENT,
    BLOCK,
    BINARY_OP,
    UNARY_OP,
    IDENTIFIER,
    NUMBER,
    FUNCTION_DEFINITION
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
    std::variant<int, double> value;
    bool isFloat;

    NumberNode(int val, int l, int c) 
        : ASTNode(ASTNodeType::NUMBER, l, c), value(val), isFloat(false) {}
    
    NumberNode(double val, int l, int c) 
        : ASTNode(ASTNodeType::NUMBER, l, c), value(val), isFloat(true) {}
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
        : ASTNode(ASTNodeType::BINARY_OP, line, col), op(o), left(std::move(l)), right(std::move(r)) {}
};

class UnaryOpNode : public ASTNode {
public:
    std::string op;
    ASTNodePtr operand;

    UnaryOpNode(const std::string& o, ASTNodePtr opnd, int line, int col)
        : ASTNode(ASTNodeType::UNARY_OP, line, col), op(o), operand(std::move(opnd)) {}
};

class AssignmentNode : public ASTNode {
public:
    std::string name;  // 变量名
    ASTNodePtr value;  // 右值表达式
    ASTNodePtr left;   // 兼容旧代码：左值表达式
    ASTNodePtr right;  // 兼容旧代码：右值表达式

    // 新构造函数（推荐）
    AssignmentNode(const std::string& n, ASTNodePtr v, int l, int c)
        : ASTNode(ASTNodeType::ASSIGNMENT, l, c), name(n), value(std::move(v)) {}
    
    // 兼容旧代码的构造函数
    AssignmentNode(ASTNodePtr l, ASTNodePtr r, int line, int col)
        : ASTNode(ASTNodeType::ASSIGNMENT, line, col), left(std::move(l)), right(std::move(r)) {}
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
          condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
};

class WhileStatementNode : public ASTNode {
public:
    ASTNodePtr condition;
    ASTNodePtr body;

    WhileStatementNode(ASTNodePtr cond, ASTNodePtr b, int line, int col)
        : ASTNode(ASTNodeType::WHILE_STATEMENT, line, col), 
          condition(std::move(cond)), body(std::move(b)) {}
};

class ForStatementNode : public ASTNode {
public:
    ASTNodePtr init;
    ASTNodePtr condition;
    ASTNodePtr increment;
    ASTNodePtr body;

    ForStatementNode(ASTNodePtr i, ASTNodePtr c, ASTNodePtr inc, ASTNodePtr b, int line, int col)
        : ASTNode(ASTNodeType::FOR_STATEMENT, line, col),
          init(std::move(i)), condition(std::move(c)), increment(std::move(inc)), body(std::move(b)) {}
};

class ReturnStatementNode : public ASTNode {
public:
    ASTNodePtr value;

    ReturnStatementNode(ASTNodePtr v, int l, int c)
        : ASTNode(ASTNodeType::RETURN_STATEMENT, l, c), value(std::move(v)) {}
};

class DeclarationNode : public ASTNode {
public:
    std::string type;       // 类型名（int, float, void）
    std::string name;       // 变量名（单变量声明）
    ASTNodePtr initializer; // 初始化表达式
    
    // 多变量声明支持
    std::vector<std::unique_ptr<IdentifierNode>> variables;

    // 单变量声明构造函数
    DeclarationNode(const std::string& t, const std::string& n, ASTNodePtr init, int l, int c)
        : ASTNode(ASTNodeType::DECLARATION, l, c), type(t), name(n), initializer(std::move(init)) {}
    
    // 多变量声明构造函数
    DeclarationNode(const std::string& t, int l, int c)
        : ASTNode(ASTNodeType::DECLARATION, l, c), type(t) {}
};

class FunctionDefinitionNode : public ASTNode {
public:
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> parameters; // (name, type)
    ASTNodePtr body;

    FunctionDefinitionNode(const std::string& n, const std::string& rt,
                           std::vector<std::pair<std::string, std::string>> params,
                           ASTNodePtr b, int l, int c)
        : ASTNode(ASTNodeType::FUNCTION_DEFINITION, l, c),
          name(n), returnType(rt), parameters(std::move(params)), body(std::move(b)) {}
};

class ProgramNode : public ASTNode {
public:
    std::vector<ASTNodePtr> declarations;  // 全局变量声明
    std::vector<ASTNodePtr> functions;     // 函数定义
    std::vector<ASTNodePtr> children;      // 兼容旧代码

    ProgramNode() : ASTNode(ASTNodeType::PROGRAM, 0, 0) {}
    
    void addDeclaration(ASTNodePtr decl) {
        declarations.push_back(std::move(decl));
    }
    
    void addFunction(ASTNodePtr func) {
        functions.push_back(std::move(func));
    }
    
    void addChild(ASTNodePtr child) {
        children.push_back(std::move(child));
    }
};