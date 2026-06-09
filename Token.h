#pragma once

#include <string>

enum class TokenType
{
    INT,
    IF,
    ELSE,
    WHILE,

    ID,
    NUM,

    PLUS,
    MINUS,
    MUL,
    DIV,

    LT,
    GT,
    EQ,
    ASSIGN,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,

    SEMI,
    COMMA,

    END,

    ERROR_TOKEN
};

struct Token
{
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType t,
          const std::string& l,
          int ln,
          int col)
        : type(t),
          lexeme(l),
          line(ln),
          column(col)
    {
    }
};

