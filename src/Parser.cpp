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
               currentToken.type != TokenType::ELSE &&
               currentToken.type != TokenType::FI) {
            currentToken = lexer.getNextToken();
        }
        if (currentToken.type == TokenType::SEMICOLON) {
            eat(TokenType::SEMICOLON);
        }
    }

    ASTNodePtr parseProgram();
    ASTNodePtr parseDeclaration();
    ASTNodePtr parseFunctionDefinition(const std::string& name, const std::string& returnType);
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
    ASTNodePtr parseBlock();
    std::string parseType();

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
        try {
            if (currentToken.type == TokenType::INT || 
                currentToken.type == TokenType::FLOAT_TYPE || 
                currentToken.type == TokenType::VOID) {
                std::string type = currentToken.value;
                eat(currentToken.type);
                
                if (currentToken.type == TokenType::IDENTIFIER) {
                    std::string name = currentToken.value;
                    eat(TokenType::IDENTIFIER);
                    
                    if (currentToken.type == TokenType::LPAREN) {
                        program->functions.push_back(parseFunctionDefinition(name, type));
                    } else {
                        program->declarations.push_back(std::make_unique<DeclarationNode>(
                            type, name, nullptr, currentToken.line, currentToken.column));
                        if (currentToken.type == TokenType::ASSIGN) {
                            eat(TokenType::ASSIGN);
                            auto init = parseExpression();
                            static_cast<DeclarationNode*>(program->declarations.back().get())->initializer = std::move(init);
                        }
                        if (currentToken.type == TokenType::SEMICOLON) {
                            eat(TokenType::SEMICOLON);
                        }
                    }
                } else {
                    throw std::runtime_error("Expected identifier after type");
                }
            } else {
                program->declarations.push_back(parseDeclaration());
            }
        } catch (const std::runtime_error& e) {
            synchronize();
        }
    }
    
    return std::move(program);
}

ASTNodePtr Parser::parseDeclaration() {
    std::string type = parseType();
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
    
    return std::make_unique<DeclarationNode>(type, name, std::move(initializer), 
                                             currentToken.line, currentToken.column);
}

std::string Parser::parseType() {
    if (currentToken.type == TokenType::INT) {
        eat(TokenType::INT);
        return "int";
    } else if (currentToken.type == TokenType::FLOAT_TYPE) {
        eat(TokenType::FLOAT_TYPE);
        return "float";
    } else if (currentToken.type == TokenType::VOID) {
        eat(TokenType::VOID);
        return "void";
    } else {
        throw std::runtime_error("Expected type");
    }
}

ASTNodePtr Parser::parseFunctionDefinition(const std::string& name, const std::string& returnType) {
    eat(TokenType::LPAREN);
    
    std::vector<std::pair<std::string, std::string>> parameters;
    if (currentToken.type != TokenType::RPAREN) {
        while (true) {
            std::string paramType = parseType();
            std::string paramName = currentToken.value;
            eat(TokenType::IDENTIFIER);
            parameters.emplace_back(paramName, paramType);
            
            if (currentToken.type == TokenType::COMMA) {
                eat(TokenType::COMMA);
            } else {
                break;
            }
        }
    }
    eat(TokenType::RPAREN);
    
    ASTNodePtr body = parseBlock();
    
    return std::make_unique<FunctionDefinitionNode>(
        name, 
        returnType, 
        parameters, 
        std::move(body), 
        currentToken.line, 
        currentToken.column
    );
}

ASTNodePtr Parser::parseBlock() {
    eat(TokenType::LBRACE);
    
    auto block = std::make_unique<BlockNode>(currentToken.line, currentToken.column);
    
    while (currentToken.type != TokenType::RBRACE && 
           currentToken.type != TokenType::END_OF_FILE) {
        try {
            block->statements.push_back(parseStatement());
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
        case TokenType::FOR:
            return parseForStatement();
        case TokenType::RETURN:
            return parseReturnStatement();
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
            } else if (currentToken.type == TokenType::LPAREN) {
                eat(TokenType::LPAREN);
                std::vector<ASTNodePtr> args;
                if (currentToken.type != TokenType::RPAREN) {
                    while (true) {
                        args.push_back(parseExpression());
                        if (currentToken.type == TokenType::COMMA) {
                            eat(TokenType::COMMA);
                        } else {
                            break;
                        }
                    }
                }
                eat(TokenType::RPAREN);
                if (currentToken.type == TokenType::SEMICOLON) {
                    eat(TokenType::SEMICOLON);
                }
                return std::make_unique<IdentifierNode>(name, currentToken.line, currentToken.column);
            } else {
                throw std::runtime_error("Unexpected token after identifier");
            }
        }
        case TokenType::INT:
        case TokenType::FLOAT_TYPE:
        case TokenType::VOID: {
            return parseDeclaration();
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

ASTNodePtr Parser::parseForStatement() {
    eat(TokenType::FOR);
    eat(TokenType::LPAREN);
    
    ASTNodePtr init = nullptr;
    if (currentToken.type != TokenType::SEMICOLON) {
        init = parseStatement();
    }
    eat(TokenType::SEMICOLON);
    
    ASTNodePtr condition = nullptr;
    if (currentToken.type != TokenType::SEMICOLON) {
        condition = parseExpression();
    }
    eat(TokenType::SEMICOLON);
    
    ASTNodePtr increment = nullptr;
    if (currentToken.type != TokenType::RPAREN) {
        increment = parseExpression();
    }
    eat(TokenType::RPAREN);
    
    auto body = parseStatement();
    
    return std::make_unique<ForStatementNode>(std::move(init), 
                                              std::move(condition), 
                                              std::move(increment), 
                                              std::move(body),
                                              currentToken.line, 
                                              currentToken.column);
}

ASTNodePtr Parser::parseReturnStatement() {
    eat(TokenType::RETURN);
    
    ASTNodePtr value = nullptr;
    if (currentToken.type != TokenType::SEMICOLON) {
        value = parseExpression();
    }
    
    if (currentToken.type == TokenType::SEMICOLON) {
        eat(TokenType::SEMICOLON);
    }
    
    return std::make_unique<ReturnStatementNode>(std::move(value), 
                                                 currentToken.line, 
                                                 currentToken.column);
}

ASTNodePtr Parser::parseExpression() {
    return parseAssignment();
}

ASTNodePtr Parser::parseAssignment() {
    auto left = parseLogicalOr();
    
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

ASTNodePtr Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    
    while (currentToken.type == TokenType::IDENTIFIER && currentToken.value == "||") {
        std::string op = currentToken.value;
        eat(TokenType::IDENTIFIER);
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseLogicalAnd() {
    auto left = parseEquality();
    
    while (currentToken.type == TokenType::IDENTIFIER && currentToken.value == "&&") {
        std::string op = currentToken.value;
        eat(TokenType::IDENTIFIER);
        auto right = parseEquality();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseEquality() {
    auto left = parseRelational();
    
    while (currentToken.type == TokenType::EQUAL || currentToken.type == TokenType::NOT_EQUAL) {
        std::string op = currentToken.value;
        eat(currentToken.type);
        auto right = parseRelational();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseRelational() {
    auto left = parseAdditive();
    
    while (currentToken.type == TokenType::LESS || currentToken.type == TokenType::GREATER ||
           currentToken.type == TokenType::LESS_EQUAL || currentToken.type == TokenType::GREATER_EQUAL) {
        std::string op = currentToken.value;
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
        std::string op = currentToken.value;
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
        std::string op = currentToken.value;
        eat(currentToken.type);
        auto right = parseUnary();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), 
                                              currentToken.line, currentToken.column);
    }
    
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (currentToken.type == TokenType::MINUS) {
        std::string op = currentToken.value;
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
    } else if (currentToken.type == TokenType::FLOAT) {
        double val = std::stod(currentToken.value);
        int line = currentToken.line;
        int col = currentToken.column;
        eat(TokenType::FLOAT);
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
