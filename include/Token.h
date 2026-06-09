#pragma once

#include <string>
#include <vector>
#include <fstream>

enum class TokenType {
    // 关键字
    INT,
    IF,
    ELSE,
    WHILE,
    
    // 标识符和数字
    ID,
    NUM,
    
    // 运算符
    PLUS,      // +
    MINUS,     // -
    MUL,       // *
    DIV,       // /
    LT,        // <
    GT,        // >
    EQ,        // ==
    ASSIGN,    // =
    
    // 界符
    LPAREN,    // (
    RPAREN,    // )
    LBRACE,    // {
    RBRACE,    // }
    SEMI,      // ;
    COMMA,     // ,
    
    // 输入结束
    END,
    
    // 错误标记
    ERROR_TOKEN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;  // 增加列号，方便精确定位错误
    
    Token(TokenType t, const std::string& l, int ln, int col)
        : type(t), lexeme(l), line(ln), column(col) {}
};

class Lexer {
private:
    std::ifstream file;
    char currentChar;
    int line;
    int column;

    void advance();
    char peek();
    bool isEOF();
    TokenType checkKeyword(const std::string& word);
    Token scanIdentifier();
    Token scanNumber();

public:
    Lexer(const std::string& source);
    ~Lexer();
    
    // 获取下一个Token（按需扫描）
    Token getNextToken();
    
    // 扫描整个源程序并返回Token流
    std::vector<Token> tokenize();
};