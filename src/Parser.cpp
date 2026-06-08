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
               << ": unexpected token '" << currentToken.value 
               << "', expected token of type " << static_cast<int>(expected);
            throw std::runtime_error(ss.str());
        }
    }

    void synchronize() {
        while (currentToken.type != TokenType::END_OF_FILE &&
               currentToken.type != TokenType::SEMICOLON &&
               currentToken.type != TokenType::RBRACE &&
               currentToken.type != TokenType::ELSE) {
            currentToken = lexer.getNextToken();
        }
        if (currentToken.type == TokenType::SEMICOLON) {
            eat(TokenType::SEMICOLON);
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
    Parser(Lexer& l) : lexer(l), currentToken(TokenType::END_OF_FILE, "", 0, 0) {
        currentToken = lexer.getNextToken();
    }

    ASTNodePtr parse() {
        return parseProgram();
    }
};

ASTNodePtr Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    
    while (currentToken.type != TokenType::END_OF_FILE) {
        try {
            if (currentToken.type == TokenType::INT) {
                program->addDeclaration(parseDeclaration());
            } else {
                program->addStatement(parseStatement());
            }
        } catch (const std::runtime_error& e) {
            synchronize();
        }
    }
    
    return std::move(program);
}

ASTNodePtr Parser::parseDeclaration() {
    eat(TokenType::INT);
    
    std::string name = currentToken.value;
    eat(TokenType::IDENTIFIER);
    
    ASTNodePtr initializer = nullptr;
    if (currentToken.type == TokenType::ASSIGN) {
        eat(TokenType::ASSIGN);
        initializer = parseExpression();
    }
    
    if (currentToken.type == TokenType::SEMICOLON) {
        eat(TokenType::SEMICOLON);
    } else {
        throw std::runtime_error("Missing semicolon in declaration");
    }
    
    return std::make_unique<DeclarationNode>("int", name, std::move(initializer), 
                                             currentToken.line, currentToken.column);
}

ASTNodePtr Parser::parseBlock() {
    eat(TokenType::LBRACE);
    
    auto block = std::make_unique<BlockNode>(currentToken.line, currentToken.column);
    
    while (currentToken.type != TokenType::RBRACE && 
           currentToken.type != TokenType::END_OF_FILE) {
        try {
            if (currentToken.type == TokenType::INT) {
                block->addStatement(parseDeclaration());
            } else {
                block->addStatement(parseStatement());
            }
        } catch (const std::runtime_error& e) {
            synchronize();
        }
    }
    
    if (currentToken.type == TokenType::RBRACE) {
        eat(TokenType::RBRACE);
    } else {
        throw std::runtime_error("Missing closing brace");
    }
    
    return std::move(block);
}

ASTNodePtr Parser::parseStatement() {
    switch (currentToken.type) {
        case TokenType::IF:
            return parseIfStatement();
        case TokenType::WHILE:
            return parseWhileStatement();
        case TokenType::LBRACE:
            return parseBlock();
        case TokenType::IDENTIFIER: {
            std::string name = currentToken.value;
            eat(TokenType::IDENTIFIER);
            if (currentToken.type == TokenType::ASSIGN) {
                eat(TokenType::ASSIGN);
                auto value = parseExpression();
                if (currentToken.type == TokenType::SEMICOLON) {
                    eat(TokenType::SEMICOLON);
                }
                return std::make_unique<AssignmentNode>(name, std::move(value), 
                                                       currentToken.line, currentToken.column);
            } else {
                throw std::runtime_error("Unexpected token after identifier");
            }
        }
        case TokenType::SEMICOLON: {
            eat(TokenType::SEMICOLON);
            return nullptr;
        }
        default: {
            auto expr = parseExpression();
            if (currentToken.type == TokenType::SEMICOLON) {
                eat(TokenType::SEMICOLON);
            }
            return expr;
        }
    }
}

ASTNodePtr Parser::parseIfStatement() {
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
    
    return std::make_unique<IfStatementNode>(std::move(condition), 
                                             std::move(thenBranch), 
                                             std::move(elseBranch),
                                             currentToken.line, 
                                             currentToken.column);
}

ASTNodePtr Parser::parseWhileStatement() {
    eat(TokenType::WHILE);
    eat(TokenType::LPAREN);
    auto condition = parseExpression();
    eat(TokenType::RPAREN);
    
    auto body = parseStatement();
    
    return std::make_unique<WhileStatementNode>(std::move(condition), 
                                                std::move(body),
                                                currentToken.line, 
                                                currentToken.column);
}

ASTNodePtr Parser::parseExpression() {
    return parseAssignment();
}

ASTNodePtr Parser::parseAssignment() {
    auto left = parseEquality();
    
    if (currentToken.type == TokenType::ASSIGN) {
        eat(TokenType::ASSIGN);
        auto right = parseAssignment();
        if (auto ident = dynamic_cast<IdentifierNode*>(left.get())) {
            return std::make_unique<AssignmentNode>(ident->name, std::move(right), 
                                                   currentToken.line, currentToken.column);
        } else {
            throw std::runtime_error("Invalid left-hand side of assignment");
        }
    }
    
    return left;
}

ASTNodePtr Parser::parseEquality() {
    auto left = parseRelational();
    
    while (currentToken.type == TokenType::EQUAL) {
        std::string op = "==";
        eat(TokenType::EQUAL);
        auto right = parseRelational();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseRelational() {
    auto left = parseAdditive();
    
    while (currentToken.type == TokenType::LESS || currentToken.type == TokenType::GREATER) {
        std::string op = (currentToken.type == TokenType::LESS) ? "<" : ">";
        eat(currentToken.type);
        auto right = parseAdditive();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseAdditive() {
    auto left = parseMultiplicative();
    
    while (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
        std::string op = (currentToken.type == TokenType::PLUS) ? "+" : "-";
        eat(currentToken.type);
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseMultiplicative() {
    auto left = parseUnary();
    
    while (currentToken.type == TokenType::MULTIPLY || currentToken.type == TokenType::DIVIDE) {
        std::string op = (currentToken.type == TokenType::MULTIPLY) ? "*" : "/";
        eat(currentToken.type);
        auto right = parseUnary();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (currentToken.type == TokenType::MINUS) {
        std::string op = "-";
        eat(TokenType::MINUS);
        auto operand = parseUnary();
        return std::make_unique<UnaryOpNode>(op, std::move(operand), 
                                             currentToken.line, currentToken.column);
    }
    
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary() {
    if (currentToken.type == TokenType::INTEGER) {
        int val = std::stoi(currentToken.value);
        int line = currentToken.line;
        int col = currentToken.column;
        eat(TokenType::INTEGER);
        return std::make_unique<NumberNode>(val, line, col);
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        std::string name = currentToken.value;
        int line = currentToken.line;
        int col = currentToken.column;
        eat(TokenType::IDENTIFIER);
        return std::make_unique<IdentifierNode>(name, line, col);
    } else if (currentToken.type == TokenType::LPAREN) {
        eat(TokenType::LPAREN);
        auto expr = parseExpression();
        eat(TokenType::RPAREN);
        return expr;
    } else {
        std::stringstream ss;
        ss << "Unexpected token '" << currentToken.value << "' at line " 
           << currentToken.line << ", column " << currentToken.column;
        throw std::runtime_error(ss.str());
    }
}