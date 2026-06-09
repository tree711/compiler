#pragma once

#include "AST.h"
#include "Lexer.h"

class Parser {
private:
    Lexer& lexer;
    Token currentToken;

    void eat(TokenType expected);
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
    explicit Parser(Lexer& lexer);

    ASTNodePtr parse();
};
