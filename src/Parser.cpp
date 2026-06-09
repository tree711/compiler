#include "../include/Parser.h"

#include <sstream>
#include <stdexcept>
#include <utility>

Parser::Parser(Lexer& lexer)
    : lexer(lexer), currentToken(TokenType::END_OF_FILE, "", 0, 0) {
    currentToken = lexer.getNextToken();
}

void Parser::eat(TokenType expected) {
    if (currentToken.type == expected) {
        currentToken = lexer.getNextToken();
        return;
    }

    std::stringstream message;
    message << "Syntax error at line " << currentToken.line
            << ", column " << currentToken.column
            << ": unexpected token '" << currentToken.value
            << "', expected token type " << static_cast<int>(expected);
    throw std::runtime_error(message.str());
}

ASTNodePtr Parser::parse() {
    return parseProgram();
}

ASTNodePtr Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    while (currentToken.type != TokenType::END_OF_FILE) {
        if (currentToken.type == TokenType::INT) {
            program->addDeclaration(parseDeclaration());
        } else {
            ASTNodePtr statement = parseStatement();
            if (statement) {
                program->addStatement(std::move(statement));
            }
        }
    }
    return program;
}

ASTNodePtr Parser::parseDeclaration() {
    const int line = currentToken.line;
    const int column = currentToken.column;
    eat(TokenType::INT);

    const std::string name = currentToken.value;
    eat(TokenType::IDENTIFIER);

    ASTNodePtr initializer;
    if (currentToken.type == TokenType::ASSIGN) {
        eat(TokenType::ASSIGN);
        initializer = parseExpression();
    }
    eat(TokenType::SEMICOLON);

    return std::make_unique<DeclarationNode>(
        "int", name, std::move(initializer), line, column);
}

ASTNodePtr Parser::parseBlock() {
    const int line = currentToken.line;
    const int column = currentToken.column;
    eat(TokenType::LBRACE);

    auto block = std::make_unique<BlockNode>(line, column);
    while (currentToken.type != TokenType::RBRACE &&
           currentToken.type != TokenType::END_OF_FILE) {
        ASTNodePtr statement;
        if (currentToken.type == TokenType::INT) {
            statement = parseDeclaration();
        } else {
            statement = parseStatement();
        }
        if (statement) {
            block->addStatement(std::move(statement));
        }
    }
    eat(TokenType::RBRACE);
    return block;
}

ASTNodePtr Parser::parseStatement() {
    switch (currentToken.type) {
        case TokenType::IF:
            return parseIfStatement();
        case TokenType::WHILE:
            return parseWhileStatement();
        case TokenType::LBRACE:
            return parseBlock();
        case TokenType::SEMICOLON:
            eat(TokenType::SEMICOLON);
            return nullptr;
        default: {
            ASTNodePtr expression = parseExpression();
            eat(TokenType::SEMICOLON);
            return expression;
        }
    }
}

ASTNodePtr Parser::parseIfStatement() {
    const int line = currentToken.line;
    const int column = currentToken.column;
    eat(TokenType::IF);
    eat(TokenType::LPAREN);
    ASTNodePtr condition = parseExpression();
    eat(TokenType::RPAREN);
    ASTNodePtr thenBranch = parseStatement();

    ASTNodePtr elseBranch;
    if (currentToken.type == TokenType::ELSE) {
        eat(TokenType::ELSE);
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStatementNode>(
        std::move(condition), std::move(thenBranch),
        std::move(elseBranch), line, column);
}

ASTNodePtr Parser::parseWhileStatement() {
    const int line = currentToken.line;
    const int column = currentToken.column;
    eat(TokenType::WHILE);
    eat(TokenType::LPAREN);
    ASTNodePtr condition = parseExpression();
    eat(TokenType::RPAREN);
    return std::make_unique<WhileStatementNode>(
        std::move(condition), parseStatement(), line, column);
}

ASTNodePtr Parser::parseExpression() {
    return parseAssignment();
}

ASTNodePtr Parser::parseAssignment() {
    ASTNodePtr left = parseEquality();
    if (currentToken.type != TokenType::ASSIGN) {
        return left;
    }

    auto* identifier = dynamic_cast<IdentifierNode*>(left.get());
    if (!identifier) {
        throw std::runtime_error("Invalid left-hand side of assignment");
    }

    const std::string name = identifier->name;
    const int line = identifier->line;
    const int column = identifier->column;
    eat(TokenType::ASSIGN);
    return std::make_unique<AssignmentNode>(
        name, parseAssignment(), line, column);
}

ASTNodePtr Parser::parseEquality() {
    ASTNodePtr left = parseRelational();
    while (currentToken.type == TokenType::EQUAL) {
        const int line = currentToken.line;
        const int column = currentToken.column;
        eat(TokenType::EQUAL);
        left = std::make_unique<BinaryOpNode>(
            "==", std::move(left), parseRelational(), line, column);
    }
    return left;
}

ASTNodePtr Parser::parseRelational() {
    ASTNodePtr left = parseAdditive();
    while (currentToken.type == TokenType::LESS ||
           currentToken.type == TokenType::GREATER) {
        const std::string op = currentToken.value;
        const int line = currentToken.line;
        const int column = currentToken.column;
        eat(currentToken.type);
        left = std::make_unique<BinaryOpNode>(
            op, std::move(left), parseAdditive(), line, column);
    }
    return left;
}

ASTNodePtr Parser::parseAdditive() {
    ASTNodePtr left = parseMultiplicative();
    while (currentToken.type == TokenType::PLUS ||
           currentToken.type == TokenType::MINUS) {
        const std::string op = currentToken.value;
        const int line = currentToken.line;
        const int column = currentToken.column;
        eat(currentToken.type);
        left = std::make_unique<BinaryOpNode>(
            op, std::move(left), parseMultiplicative(), line, column);
    }
    return left;
}

ASTNodePtr Parser::parseMultiplicative() {
    ASTNodePtr left = parseUnary();
    while (currentToken.type == TokenType::MULTIPLY ||
           currentToken.type == TokenType::DIVIDE) {
        const std::string op = currentToken.value;
        const int line = currentToken.line;
        const int column = currentToken.column;
        eat(currentToken.type);
        left = std::make_unique<BinaryOpNode>(
            op, std::move(left), parseUnary(), line, column);
    }
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (currentToken.type != TokenType::MINUS) {
        return parsePrimary();
    }

    const int line = currentToken.line;
    const int column = currentToken.column;
    eat(TokenType::MINUS);
    return std::make_unique<UnaryOpNode>("-", parseUnary(), line, column);
}

ASTNodePtr Parser::parsePrimary() {
    const int line = currentToken.line;
    const int column = currentToken.column;

    if (currentToken.type == TokenType::INTEGER) {
        const int value = std::stoi(currentToken.value);
        eat(TokenType::INTEGER);
        return std::make_unique<NumberNode>(value, line, column);
    }
    if (currentToken.type == TokenType::IDENTIFIER) {
        const std::string name = currentToken.value;
        eat(TokenType::IDENTIFIER);
        return std::make_unique<IdentifierNode>(name, line, column);
    }
    if (currentToken.type == TokenType::LPAREN) {
        eat(TokenType::LPAREN);
        ASTNodePtr expression = parseExpression();
        eat(TokenType::RPAREN);
        return expression;
    }

    throw std::runtime_error(
        "Expected expression at line " + std::to_string(line) +
        ", column " + std::to_string(column));
}
