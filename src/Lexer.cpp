#include "../include/Lexer.h"

#include <cctype>
#include <stdexcept>

void Lexer::advance() {
    file.get(currentChar);
    if (currentChar == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
}

bool Lexer::isEOF() {
    return file.eof();
}

TokenType Lexer::checkKeyword(const std::string& word) {
    if (word == "if") return TokenType::IF;
    if (word == "else") return TokenType::ELSE;
    if (word == "while") return TokenType::WHILE;
    if (word == "int") return TokenType::INT;
    return TokenType::IDENTIFIER;
}

Token Lexer::scanIdentifier() {
    const int startLine = line;
    const int startColumn = column;
    std::string value;
    while (!isEOF() &&
           (std::isalnum(static_cast<unsigned char>(currentChar)) ||
            currentChar == '_')) {
        value += currentChar;
        advance();
    }
    return Token(checkKeyword(value), value, startLine, startColumn);
}

Token Lexer::scanNumber() {
    const int startLine = line;
    const int startColumn = column;
    std::string value;
    while (!isEOF() && std::isdigit(static_cast<unsigned char>(currentChar))) {
        value += currentChar;
        advance();
    }
    return Token(TokenType::INTEGER, value, startLine, startColumn);
}

Lexer::Lexer(const std::string& filename)
    : currentChar('\0'), line(1), column(0) {
    file.open(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    advance();
}

Lexer::~Lexer() {
    file.close();
}

Token Lexer::getNextToken() {
    while (!isEOF() && std::isspace(static_cast<unsigned char>(currentChar))) {
        advance();
    }
    if (isEOF()) {
        return Token(TokenType::END_OF_FILE, "", line, column);
    }
    if (std::isalpha(static_cast<unsigned char>(currentChar)) ||
        currentChar == '_') {
        return scanIdentifier();
    }
    if (std::isdigit(static_cast<unsigned char>(currentChar))) {
        return scanNumber();
    }

    const int tokenLine = line;
    const int tokenColumn = column;
    const char c = currentChar;
    advance();
    switch (c) {
        case '+': return Token(TokenType::PLUS, "+", tokenLine, tokenColumn);
        case '-': return Token(TokenType::MINUS, "-", tokenLine, tokenColumn);
        case '*': return Token(TokenType::MULTIPLY, "*", tokenLine, tokenColumn);
        case '/': return Token(TokenType::DIVIDE, "/", tokenLine, tokenColumn);
        case '=':
            if (currentChar == '=') {
                advance();
                return Token(TokenType::EQUAL, "==", tokenLine, tokenColumn);
            }
            return Token(TokenType::ASSIGN, "=", tokenLine, tokenColumn);
        case '<': return Token(TokenType::LESS, "<", tokenLine, tokenColumn);
        case '>': return Token(TokenType::GREATER, ">", tokenLine, tokenColumn);
        case ';': return Token(TokenType::SEMICOLON, ";", tokenLine, tokenColumn);
        case ',': return Token(TokenType::COMMA, ",", tokenLine, tokenColumn);
        case '(': return Token(TokenType::LPAREN, "(", tokenLine, tokenColumn);
        case ')': return Token(TokenType::RPAREN, ")", tokenLine, tokenColumn);
        case '{': return Token(TokenType::LBRACE, "{", tokenLine, tokenColumn);
        case '}': return Token(TokenType::RBRACE, "}", tokenLine, tokenColumn);
        default:
            throw std::runtime_error(
                "Unexpected character '" + std::string(1, c) +
                "' at line " + std::to_string(tokenLine) +
                ", column " + std::to_string(tokenColumn));
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (Token token = getNextToken();
         token.type != TokenType::END_OF_FILE;
         token = getNextToken()) {
        tokens.push_back(token);
    }
    return tokens;
}
