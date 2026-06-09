#include "../include/Token.h"
#include <cctype>

Lexer::Lexer(const std::string& filename) : line(1), column(1) {
    file.open(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    advance();
}

Lexer::~Lexer() {
    file.close();
}

void Lexer::advance() {
    file.get(currentChar);
    if (currentChar == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
}

char Lexer::peek() {
    char c = file.peek();
    return c;
}

bool Lexer::isEOF() {
    return file.eof();
}

TokenType Lexer::checkKeyword(const std::string& word) {
    if (word == "if") return TokenType::IF;
    if (word == "else") return TokenType::ELSE;
    if (word == "while") return TokenType::WHILE;
    if (word == "int") return TokenType::INT;
    return TokenType::ID;
}

Token Lexer::scanIdentifier() {
    std::string lexeme;
    while (isalnum(currentChar) || currentChar == '_') {
        lexeme += currentChar;
        advance();
    }
    TokenType type = checkKeyword(lexeme);
    return Token(type, lexeme, line, column);
}

Token Lexer::scanNumber() {
    std::string lexeme;
    
    while (isdigit(currentChar)) {
        lexeme += currentChar;
        advance();
    }
    
    return Token(TokenType::NUM, lexeme, line, column);
}

Token Lexer::getNextToken() {
    while (!isEOF() && isspace(currentChar)) {
        advance();
    }

    if (isEOF()) {
        return Token(TokenType::END, "", line, column);
    }

    if (isalpha(currentChar) || currentChar == '_') {
        return scanIdentifier();
    }

    if (isdigit(currentChar)) {
        return scanNumber();
    }

    char c = currentChar;
    advance();

    switch (c) {
        case '+': return Token(TokenType::PLUS, "+", line, column);
        case '-': return Token(TokenType::MINUS, "-", line, column);
        case '*': return Token(TokenType::MUL, "*", line, column);
        case '/': return Token(TokenType::DIV, "/", line, column);
        case '=': {
            if (currentChar == '=') {
                advance();
                return Token(TokenType::EQ, "==", line, column);
            }
            return Token(TokenType::ASSIGN, "=", line, column);
        }
        case '<': return Token(TokenType::LT, "<", line, column);
        case '>': return Token(TokenType::GT, ">", line, column);
        case ';': return Token(TokenType::SEMI, ";", line, column);
        case ',': return Token(TokenType::COMMA, ",", line, column);
        case '(': return Token(TokenType::LPAREN, "(", line, column);
        case ')': return Token(TokenType::RPAREN, ")", line, column);
        case '{': return Token(TokenType::LBRACE, "{", line, column);
        case '}': return Token(TokenType::RBRACE, "}", line, column);
        default:
            return Token(TokenType::ERROR_TOKEN, std::string(1, c), line, column);
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token token = getNextToken();
    while (token.type != TokenType::END) {
        tokens.push_back(token);
        token = getNextToken();
    }
    return tokens;
}