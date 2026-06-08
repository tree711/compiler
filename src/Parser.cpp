#include "../include/Token.h"
#include "../include/AST.h"
#include <memory>

class Parser {
private:
    Lexer& lexer;
    Token currentToken;

    void eat(TokenType expected) {
        if (currentToken.type == expected) {
            currentToken = lexer.getNextToken();
        } else {
            throw std::runtime_error("Unexpected token");
        }
    }

    ASTNodePtr parseProgram();
    ASTNodePtr parseDeclaration();
    ASTNodePtr parseFunctionDefinition();
    ASTNodePtr parseStatement();
    ASTNodePtr parseExpression();
    ASTNodePtr parseAssignment();
    ASTNodePtr parseLogicalOr();
    ASTNodePtr parseLogicalAnd();
    ASTNodePtr parseEquality();
    ASTNodePtr parseRelational();
    ASTNodePtr parseAdditive();
    ASTNodePtr parseMultiplicative();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();

public:
    Parser(Lexer& l) : lexer(l) {
        currentToken = lexer.getNextToken();
    }

    ASTNodePtr parse() {
        return parseProgram();
    }
};

ASTNodePtr Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    
    while (currentToken.type != TokenType::END_OF_FILE) {
        if (currentToken.type == TokenType::INT || 
            currentToken.type == TokenType::FLOAT_TYPE || 
            currentToken.type == TokenType::VOID) {
            auto type = currentToken.value;
            eat(currentToken.type);
            
            if (currentToken.type == TokenType::IDENTIFIER) {
                eat(TokenType::IDENTIFIER);
                if (currentToken.type == TokenType::LPAREN) {
                    program->functions.push_back(parseFunctionDefinition());
                } else {
                    currentToken = lexer.getNextToken();
                    program->declarations.push_back(parseDeclaration());
                }
            }
        } else {
            program->declarations.push_back(parseDeclaration());
        }
    }
    
    return std::move(program);
}

ASTNodePtr Parser::parseDeclaration() {
    return nullptr;
}

ASTNodePtr Parser::parseFunctionDefinition() {
    return nullptr;
}

ASTNodePtr Parser::parseStatement() {
    return nullptr;
}

ASTNodePtr Parser::parseExpression() {
    return parseAssignment();
}

ASTNodePtr Parser::parseAssignment() {
    return parseLogicalOr();
}

ASTNodePtr Parser::parseLogicalOr() {
    return parseLogicalAnd();
}

ASTNodePtr Parser::parseLogicalAnd() {
    return parseEquality();
}

ASTNodePtr Parser::parseEquality() {
    return parseRelational();
}

ASTNodePtr Parser::parseRelational() {
    return parseAdditive();
}

ASTNodePtr Parser::parseAdditive() {
    return parseMultiplicative();
}

ASTNodePtr Parser::parseMultiplicative() {
    return parseUnary();
}

ASTNodePtr Parser::parseUnary() {
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary() {
    if (currentToken.type == TokenType::INTEGER) {
        int val = std::stoi(currentToken.value);
        eat(TokenType::INTEGER);
        return std::make_unique<NumberNode>(val, currentToken.line, currentToken.column);
    } else if (currentToken.type == TokenType::FLOAT) {
        double val = std::stod(currentToken.value);
        eat(TokenType::FLOAT);
        return std::make_unique<NumberNode>(val, currentToken.line, currentToken.column);
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        std::string name = currentToken.value;
        eat(TokenType::IDENTIFIER);
        return std::make_unique<IdentifierNode>(name, currentToken.line, currentToken.column);
    } else if (currentToken.type == TokenType::LPAREN) {
        eat(TokenType::LPAREN);
        auto expr = parseExpression();
        eat(TokenType::RPAREN);
        return expr;
    }
    return nullptr;
}
