/**
 * TinyC 编译器主程序
 * 
 * 完整的编译流程：源代码 → 词法分析 → 语法分析 → 语义分析 → 中间代码 → 优化 → 汇编代码
 * 
 * 用法: ./compiler <源文件> [-o 输出文件] [-v] [-O]
 *   -o: 指定输出文件名（默认 output.s）
 *   -v: 详细模式，显示各阶段输出
 *   -O: 启用优化
 */

#include "../include/Token.h"
#include "../include/AST.h"
#include "../include/Symbol.h"
#include "../include/IR.h"
#include "../include/IRGenerator.h"
#include "../include/Optimizer.h"
#include "../include/CodeGenerator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
extern "C" __declspec(dllimport) int __stdcall SetConsoleCP(unsigned int);
#endif

// Parser 类声明（实现在 Parser.cpp）
class Parser {
private:
    Lexer& lexer;
    Token currentToken;

    void eat(TokenType expected) {
        if (currentToken.type == expected) {
            currentToken = lexer.getNextToken();
        } else {
            std::stringstream ss;
            ss << "Syntax error at line " << currentToken.line << ", column " << currentToken.column 
               << ": unexpected token '" << currentToken.lexeme 
               << "', expected token of type " << static_cast<int>(expected);
            throw std::runtime_error(ss.str());
        }
    }

    ASTNodePtr parseProgram();
    ASTNodePtr parseDeclaration();
    ASTNodePtr parseStatement();
    ASTNodePtr parseExpression();
    ASTNodePtr parseAssignment();
    ASTNodePtr parseEquality();
    ASTNodePtr parseRelational();
    ASTNodePtr parseAdditive();
    ASTNodePtr parseMultiplicative();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();
    ASTNodePtr parseBlock();
    ASTNodePtr parseIfStatement();
    ASTNodePtr parseWhileStatement();

public:
    Parser(Lexer& l) : lexer(l), currentToken(TokenType::END, "", 0, 0) {
        currentToken = lexer.getNextToken();
    }

    ASTNodePtr parse() {
        return parseProgram();
    }
};

// SemanticAnalyzer 类声明（实现在 Semantic.cpp）
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
    
    SymbolTable& getSymbolTable() { return symbolTable; }
};

// ========== 配置选项 ==========
struct CompilerOptions {
    std::string inputFile;
    std::string outputFile = "output.s";
    bool verbose = false;
    bool optimize = false;
};

// ========== 辅助函数 ==========
void printUsage(const char* programName) {
    std::cout << "TinyC 编译器 v1.0\n";
    std::cout << "用法: " << programName << " <源文件> [-o 输出文件] [-v] [-O]\n";
    std::cout << "选项:\n";
    std::cout << "  -o <文件>  指定输出文件名（默认 output.s）\n";
    std::cout << "  -v         详细模式，显示各阶段输出\n";
    std::cout << "  -O         启用优化\n";
    std::cout << "\n示例:\n";
    std::cout << "  " << programName << " test.c\n";
    std::cout << "  " << programName << " test.c -o program.s -v -O\n";
}

void printSeparator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

void printTokenList(const std::vector<Token>& tokens) {
    std::cout << "Token 列表:\n";
    for (const Token& token : tokens) {
        std::cout << "  Line " << token.line << ", Col " << token.column 
                  << ": Type=" << static_cast<int>(token.type) 
                  << ", Value='" << token.lexeme << "'\n";
    }
    std::cout << "共 " << tokens.size() << " 个 Token\n";
}

void printAST(ASTNode* node, int indent = 0) {
    if (!node) return;
    
    std::string prefix(indent * 2, ' ');
    
    switch (node->type) {
        case ASTNodeType::PROGRAM: {
            auto prog = static_cast<ProgramNode*>(node);
            std::cout << prefix << "Program\n";
            for (auto& child : prog->children) {
                printAST(child.get(), indent + 1);
            }
            break;
        }
        case ASTNodeType::DECLARATION: {
            auto decl = static_cast<DeclarationNode*>(node);
            std::cout << prefix << "Declaration: " << decl->type << " ";
            for (size_t i = 0; i < decl->variables.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << decl->variables[i]->name;
            }
            std::cout << "\n";
            break;
        }
        case ASTNodeType::ASSIGNMENT: {
            auto assign = static_cast<AssignmentNode*>(node);
            std::cout << prefix << "Assignment: ";
            if (assign->left) {
                printAST(assign->left.get());
            } else {
                std::cout << assign->name;
            }
            std::cout << " = ";
            printAST(assign->value ? assign->value.get() : assign->right.get());
            std::cout << "\n";
            break;
        }
        case ASTNodeType::IF_STATEMENT: {
            auto ifNode = static_cast<IfStatementNode*>(node);
            std::cout << prefix << "IfStatement\n";
            std::cout << prefix << "  Condition: ";
            printAST(ifNode->condition.get());
            std::cout << "\n";
            std::cout << prefix << "  Then:\n";
            printAST(ifNode->thenBranch.get(), indent + 2);
            if (ifNode->elseBranch) {
                std::cout << prefix << "  Else:\n";
                printAST(ifNode->elseBranch.get(), indent + 2);
            }
            break;
        }
        case ASTNodeType::WHILE_STATEMENT: {
            auto whileNode = static_cast<WhileStatementNode*>(node);
            std::cout << prefix << "WhileStatement\n";
            std::cout << prefix << "  Condition: ";
            printAST(whileNode->condition.get());
            std::cout << "\n";
            std::cout << prefix << "  Body:\n";
            printAST(whileNode->body.get(), indent + 2);
            break;
        }
        case ASTNodeType::BLOCK: {
            auto block = static_cast<BlockNode*>(node);
            std::cout << prefix << "Block {\n";
            for (auto& stmt : block->statements) {
                printAST(stmt.get(), indent + 1);
            }
            std::cout << prefix << "}\n";
            break;
        }
        case ASTNodeType::BINARY_OP: {
            auto binOp = static_cast<BinaryOpNode*>(node);
            std::cout << "(" << binOp->op << " ";
            printAST(binOp->left.get());
            std::cout << " ";
            printAST(binOp->right.get());
            std::cout << ")";
            break;
        }
        case ASTNodeType::UNARY_OP: {
            auto unaryOp = static_cast<UnaryOpNode*>(node);
            std::cout << "(" << unaryOp->op << " ";
            printAST(unaryOp->operand.get());
            std::cout << ")";
            break;
        }
        case ASTNodeType::IDENTIFIER: {
            auto ident = static_cast<IdentifierNode*>(node);
            std::cout << ident->name;
            break;
        }
        case ASTNodeType::NUMBER: {
            auto num = static_cast<NumberNode*>(node);
            if (num->isFloat) {
                std::cout << std::get<double>(num->value);
            } else {
                std::cout << std::get<int>(num->value);
            }
            break;
        }
        case ASTNodeType::FOR_STATEMENT:
        case ASTNodeType::RETURN_STATEMENT:
        case ASTNodeType::FUNCTION_DEFINITION:
            // 暂不支持
            break;
    }
}

void printSymbolTable(SymbolTable& symbolTable) {
    std::cout << "符号表内容:\n";
    // 由于 SymbolTable 没有提供遍历接口，这里只显示基本信息
    std::cout << "  当前作用域层级: " << symbolTable.getCurrentScope() << "\n";
}

void printIR(IRProgram& program) {
    for (auto& func : program.functions) {
        std::cout << "Function: " << func->name << "\n";
        for (auto& block : func->blocks) {
            std::cout << "  Block: " << block->name << "\n";
            for (auto& instr : block->instructions) {
                std::cout << "    ";
                if (!instr->label.empty()) {
                    std::cout << instr->label << ": ";
                }
                if (!instr->result.empty()) {
                    std::cout << instr->result << " = ";
                }
                switch (instr->opcode) {
                    case IROpcode::ADD:      std::cout << "ADD"; break;
                    case IROpcode::SUB:      std::cout << "SUB"; break;
                    case IROpcode::MUL:      std::cout << "MUL"; break;
                    case IROpcode::DIV:      std::cout << "DIV"; break;
                    case IROpcode::LOAD:     std::cout << "LOAD"; break;
                    case IROpcode::STORE:    std::cout << "STORE"; break;
                    case IROpcode::CONST:    std::cout << "CONST"; break;
                    case IROpcode::BRANCH:   std::cout << "BRANCH"; break;
                    case IROpcode::BRANCH_EQ: std::cout << "BRANCH_EQ"; break;
                    case IROpcode::BRANCH_NE: std::cout << "BRANCH_NE"; break;
                    case IROpcode::BRANCH_LT: std::cout << "BRANCH_LT"; break;
                    case IROpcode::BRANCH_GT: std::cout << "BRANCH_GT"; break;
                    case IROpcode::BRANCH_LE: std::cout << "BRANCH_LE"; break;
                    case IROpcode::BRANCH_GE: std::cout << "BRANCH_GE"; break;
                    case IROpcode::CALL:     std::cout << "CALL"; break;
                    case IROpcode::RET:      std::cout << "RET"; break;
                    case IROpcode::PHI:      std::cout << "PHI"; break;
                    case IROpcode::CMP_EQ:   std::cout << "CMP_EQ"; break;
                    case IROpcode::CMP_NE:   std::cout << "CMP_NE"; break;
                    case IROpcode::CMP_LT:   std::cout << "CMP_LT"; break;
                    case IROpcode::CMP_GT:   std::cout << "CMP_GT"; break;
                    case IROpcode::CMP_LE:   std::cout << "CMP_LE"; break;
                    case IROpcode::CMP_GE:   std::cout << "CMP_GE"; break;
                    default:                 std::cout << "UNKNOWN"; break;
                }
                for (const auto& op : instr->operands) {
                    std::cout << " " << op;
                }
                std::cout << "\n";
            }
        }
    }
}

// ========== 主编译函数 ==========
int compile(const CompilerOptions& options) {
    try {
        // ========== Step 1: 词法分析 ==========
        if (options.verbose) printSeparator("Step 1: Lexer (词法分析)");
        
        Lexer lexer(options.inputFile);
        
        if (options.verbose) {
            std::vector<Token> tokens = lexer.tokenize();
            printTokenList(tokens);
            std::cout << "✅ 词法分析完成\n";
        }
        
        // ========== Step 2: 语法分析 ==========
        if (options.verbose) printSeparator("Step 2: Parser (语法分析)");
        
        Lexer lexer2(options.inputFile);
        Parser parser(lexer2);
        ASTNodePtr ast = parser.parse();
        
        if (options.verbose) {
            std::cout << "AST 结构:\n";
            printAST(ast.get());
            std::cout << "\n✅ 语法分析完成\n";
        }
        
        // ========== Step 3: 语义分析 ==========
        if (options.verbose) printSeparator("Step 3: Semantic (语义分析)");
        
        SemanticAnalyzer semantic;
        semantic.analyze(ast.get());
        
        if (options.verbose) {
            printSymbolTable(semantic.getSymbolTable());
            std::cout << "✅ 语义分析完成\n";
        }
        
        // ========== Step 4: 中间代码生成 ==========
        if (options.verbose) printSeparator("Step 4: IR Generation (中间代码生成)");
        
        IRGenerator irGen;
        IRProgram ir = irGen.generate(ast.get());
        
        if (options.verbose) {
            printIR(ir);
            std::cout << "\n✅ 中间代码生成完成\n";
        }
        
        // ========== Step 5: 优化 ==========
        if (options.optimize) {
            if (options.verbose) printSeparator("Step 5: Optimization (代码优化)");
            
            Optimizer optimizer;
            optimizer.optimize(ir);
            
            if (options.verbose) {
                std::cout << "优化后的中间代码:\n";
                printIR(ir);
                std::cout << "\n✅ 代码优化完成\n";
            }
        }
        
        // ========== Step 6: 代码生成 ==========
        if (options.verbose) printSeparator("Step 6: Code Generation (代码生成)");
        
        CodeGenerator codeGen(options.outputFile);
        codeGen.generate(ir);
        
        if (options.verbose) {
            std::cout << "✅ 汇编代码已写入: " << options.outputFile << "\n";
            
            // 读取并显示生成的汇编代码
            std::ifstream inFile(options.outputFile);
            if (inFile.is_open()) {
                std::cout << "\n生成的汇编代码:\n";
                std::string line;
                while (std::getline(inFile, line)) {
                    std::cout << "  " << line << "\n";
                }
                inFile.close();
            }
        }
        
        // ========== 完成 ==========
        if (options.verbose) {
            printSeparator("编译完成");
        }
        std::cout << "✅ 编译成功: " << options.inputFile << " → " << options.outputFile << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 编译错误: " << e.what() << "\n";
        return 1;
    }
}

// ========== 主函数 ==========
int main(int argc, char* argv[]) {
#ifdef _WIN32
    constexpr unsigned int UTF8_CODE_PAGE = 65001;
    SetConsoleOutputCP(UTF8_CODE_PAGE);
    SetConsoleCP(UTF8_CODE_PAGE);
#endif

    // 解析命令行参数
    CompilerOptions options;
    
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    options.inputFile = argv[1];
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0) {
            options.verbose = true;
        } else if (strcmp(argv[i], "-O") == 0) {
            options.optimize = true;
        } else {
            std::cerr << "未知选项: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // 执行编译
    return compile(options);
}
