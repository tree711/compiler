#include "../include/Token.h"
#include "../include/AST.h"
#include <memory>
#include <stdexcept>
#include <sstream>

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

    void synchronize() {
        while (currentToken.type != TokenType::END &&
               currentToken.type != TokenType::SEMI &&
               currentToken.type != TokenType::RBRACE &&
               currentToken.type != TokenType::ELSE) {
            currentToken = lexer.getNextToken();
        }
        if (currentToken.type == TokenType::SEMI) {
            eat(TokenType::SEMI);
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

ASTNodePtr Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    
    while (currentToken.type != TokenType::END) {
        if (currentToken.type == TokenType::INT) {
            program->children.push_back(parseDeclaration());
        } else {
            program->children.push_back(parseStatement());
        }
    }
    
    return program;
}

ASTNodePtr Parser::parseDeclaration() {
    int line = currentToken.line;
    int col = currentToken.column;
    eat(TokenType::INT);
    
    auto decl = std::make_unique<DeclarationNode>("int", line, col);
    
    while (currentToken.type == TokenType::ID) {
        auto var = std::make_unique<IdentifierNode>(currentToken.lexeme, currentToken.line, currentToken.column);
        decl->variables.push_back(std::move(var));
        
        eat(TokenType::ID);
        
        if (currentToken.type == TokenType::COMMA) {
            eat(TokenType::COMMA);
        } else {
            break;
        }
    }
    
    eat(TokenType::SEMI);
    
    return decl;
}

ASTNodePtr Parser::parseStatement() {
    switch (currentToken.type) {
        case TokenType::IF:
            return parseIfStatement();
        case TokenType::WHILE:
            return parseWhileStatement();
        case TokenType::LBRACE:
            return parseBlock();
        case TokenType::ID:
        default:
            return parseAssignment();
    }
}

ASTNodePtr Parser::parseBlock() {
    int line = currentToken.line;
    int col = currentToken.column;
    eat(TokenType::LBRACE);
    
    auto block = std::make_unique<BlockNode>(line, col);
    
    while (currentToken.type != TokenType::RBRACE && 
           currentToken.type != TokenType::END) {
        if (currentToken.type == TokenType::INT) {
            block->statements.push_back(parseDeclaration());
        } else {
            block->statements.push_back(parseStatement());
        }
    }
    
    eat(TokenType::RBRACE);
    
    return block;
}

ASTNodePtr Parser::parseIfStatement() {
    int line = currentToken.line;
    int col = currentToken.column;
    eat(TokenType::IF);
    eat(TokenType::LPAREN);
    
    auto condition = parseExpression();
    
    eat(TokenType::RPAREN);
    auto thenBranch = parseStatement();
    
    ASTNodePtr elseBranch = nullptr;
    if (currentToken.type == TokenType::ELSE) {
        eat(TokenType::ELSE);
        elseBranch = parseStatement();
    }
    
    return std::make_unique<IfStatementNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line, col);
}

ASTNodePtr Parser::parseWhileStatement() {
    int line = currentToken.line;
    int col = currentToken.column;
    eat(TokenType::WHILE);
    eat(TokenType::LPAREN);
    
    auto condition = parseExpression();
    
    eat(TokenType::RPAREN);
    auto body = parseStatement();
    
    return std::make_unique<WhileStatementNode>(std::move(condition), std::move(body), line, col);
}

ASTNodePtr Parser::parseAssignment() {
    if (currentToken.type != TokenType::ID) {
        return parseExpression();
    }
    
    int line = currentToken.line;
    int col = currentToken.column;
    auto left = std::make_unique<IdentifierNode>(currentToken.lexeme, currentToken.line, currentToken.column);
    
    eat(TokenType::ID);
    eat(TokenType::ASSIGN);
    auto right = parseExpression();
    eat(TokenType::SEMI);
    
    return std::make_unique<AssignmentNode>(std::move(left), std::move(right), line, col);
}

ASTNodePtr Parser::parseExpression() {
    return parseEquality();
}

ASTNodePtr Parser::parseEquality() {
    ASTNodePtr left = parseRelational();
    
    while (currentToken.type == TokenType::EQ) {
        int line = currentToken.line;
        int col = currentToken.column;
        auto op = currentToken.lexeme;
        eat(TokenType::EQ);
        auto right = parseRelational();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), line, col);
    }
    
    return left;
}

ASTNodePtr Parser::parseRelational() {
    ASTNodePtr left = parseAdditive();
    
    while (currentToken.type == TokenType::LT || currentToken.type == TokenType::GT) {
        int line = currentToken.line;
        int col = currentToken.column;
        auto op = currentToken.lexeme;
        if (currentToken.type == TokenType::LT) {
            eat(TokenType::LT);
        } else {
            eat(TokenType::GT);
        }
        auto right = parseAdditive();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), line, col);
    }
    
    return left;
}

ASTNodePtr Parser::parseAdditive() {
    ASTNodePtr left = parseMultiplicative();
    
    while (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
        int line = currentToken.line;
        int col = currentToken.column;
        auto op = currentToken.lexeme;
        if (currentToken.type == TokenType::PLUS) {
            eat(TokenType::PLUS);
        } else {
            eat(TokenType::MINUS);
        }
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), line, col);
    }
    
    return left;
}

ASTNodePtr Parser::parseMultiplicative() {
    ASTNodePtr left = parseUnary();
    
    while (currentToken.type == TokenType::MUL || currentToken.type == TokenType::DIV) {
        int line = currentToken.line;
        int col = currentToken.column;
        auto op = currentToken.lexeme;
        if (currentToken.type == TokenType::MUL) {
            eat(TokenType::MUL);
        } else {
            eat(TokenType::DIV);
        }
        auto right = parseUnary();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), line, col);
    }
    
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (currentToken.type == TokenType::MINUS) {
        int line = currentToken.line;
        int col = currentToken.column;
        eat(TokenType::MINUS);
        auto operand = parseUnary();
        return std::make_unique<UnaryOpNode>("-", std::move(operand), line, col);
    }
    
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary() {
    if (currentToken.type == TokenType::ID) {
        auto ident = std::make_unique<IdentifierNode>(currentToken.lexeme, currentToken.line, currentToken.column);
        eat(TokenType::ID);
        return ident;
    }
    
    if (currentToken.type == TokenType::NUM) {
        auto num = std::make_unique<NumberNode>(std::stoi(currentToken.lexeme), currentToken.line, currentToken.column);
        eat(TokenType::NUM);
        return num;
    }
    
    if (currentToken.type == TokenType::LPAREN) {
        eat(TokenType::LPAREN);
        auto expr = parseExpression();
        eat(TokenType::RPAREN);
        return expr;
    }
    
    std::stringstream ss;
    ss << "Unexpected token at line " << currentToken.line 
       << ", column " << currentToken.column 
       << ": " << currentToken.lexeme;
    throw std::runtime_error(ss.str());
}