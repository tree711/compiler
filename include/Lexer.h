#pragma once

#include "Token.h"

#include <fstream>
#include <string>
#include <vector>

class Lexer {
private:
    std::ifstream file;
    char currentChar;
    int line;
    int column;

    void advance();
    bool isEOF();
    TokenType checkKeyword(const std::string& word);
    Token scanIdentifier();
    Token scanNumber();

public:
    explicit Lexer(const std::string& filename);
    ~Lexer();

    Token getNextToken();
    std::vector<Token> tokenize();
};
