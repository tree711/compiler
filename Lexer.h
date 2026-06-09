#pragma once

#include "Token.h"

#include <string>
#include <vector>
#include <unordered_map>

class Lexer
{
private:

    std::string source;

    size_t pos;

    int line;

    int column;

    std::unordered_map<std::string, TokenType> keywords;

public:

    Lexer(const std::string& src);

    Token getNextToken();

    std::vector<Token> tokenize();

private:

    char peek() const;

    char advance();

    bool match(char expected);

    bool isAtEnd() const;

    void skipWhitespaceAndComments();

    Token number();

    Token identifier();
};
