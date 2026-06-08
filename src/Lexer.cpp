#include "../include/Token.h"
#include <fstream>
#include <cctype>

class Lexer {
private:
    std::ifstream file;
    char currentChar;
    int line;
    int column;

    void advance() {
        file.get(currentChar);
        if (currentChar == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    char peek() {
        char c = file.peek();
        return c;
    }

    bool isEOF() {
        return file.eof();
    }

    TokenType checkKeyword(const std::string& word) {
        if (word == "if") return TokenType::IF;
        if (word == "else") return TokenType::ELSE;
        if (word == "while") return TokenType::WHILE;
        if (word == "for") return TokenType::FOR;
        if (word == "return") return TokenType::RETURN;
        if (word == "int") return TokenType::INT;
        if (word == "float") return TokenType::FLOAT_TYPE;
        if (word == "void") return TokenType::VOID;
        return TokenType::IDENTIFIER;
    }

    Token scanIdentifier() {
        std::string value;
        while (isalnum(currentChar) || currentChar == '_') {
            value += currentChar;
            advance();
        }
        TokenType type = checkKeyword(value);
        return Token(type, value, line, column);
    }

    Token scanNumber() {
        std::string value;
        bool isFloat = false;
        
        while (isdigit(currentChar)) {
            value += currentChar;
            advance();
        }
        
        if (currentChar == '.') {
            isFloat = true;
            value += currentChar;
            advance();
            while (isdigit(currentChar)) {
                value += currentChar;
                advance();
            }
        }
        
        if (isFloat) {
            return Token(TokenType::FLOAT, value, line, column);
        }
        return Token(TokenType::INTEGER, value, line, column);
    }

    Token scanString() {
        std::string value;
        advance();
        while (currentChar != '"' && !isEOF()) {
            if (currentChar == '\\') {
                advance();
                if (currentChar == 'n') value += '\n';
                else if (currentChar == 't') value += '\t';
                else value += currentChar;
            } else {
                value += currentChar;
            }
            advance();
        }
        advance();
        return Token(TokenType::STRING, value, line, column);
    }

public:
    Lexer(const std::string& filename) : line(1), column(1) {
        file.open(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        advance();
    }

    ~Lexer() {
        file.close();
    }

    Token getNextToken() {
        while (!isEOF() && isspace(currentChar)) {
            advance();
        }

        if (isEOF()) {
            return Token(TokenType::END_OF_FILE, "", line, column);
        }

        if (isalpha(currentChar) || currentChar == '_') {
            return scanIdentifier();
        }

        if (isdigit(currentChar)) {
            return scanNumber();
        }

        if (currentChar == '"') {
            return scanString();
        }

        char c = currentChar;
        advance();

        switch (c) {
            case '+': return Token(TokenType::PLUS, "+", line, column);
            case '-': return Token(TokenType::MINUS, "-", line, column);
            case '*': return Token(TokenType::MULTIPLY, "*", line, column);
            case '/': return Token(TokenType::DIVIDE, "/", line, column);
            case '=': {
                if (currentChar == '=') {
                    advance();
                    return Token(TokenType::EQUAL, "==", line, column);
                }
                return Token(TokenType::ASSIGN, "=", line, column);
            }
            case '!': {
                if (currentChar == '=') {
                    advance();
                    return Token(TokenType::NOT_EQUAL, "!=", line, column);
                }
                break;
            }
            case '<': {
                if (currentChar == '=') {
                    advance();
                    return Token(TokenType::LESS_EQUAL, "<=", line, column);
                }
                return Token(TokenType::LESS, "<", line, column);
            }
            case '>': {
                if (currentChar == '=') {
                    advance();
                    return Token(TokenType::GREATER_EQUAL, ">=", line, column);
                }
                return Token(TokenType::GREATER, ">", line, column);
            }
            case ';': return Token(TokenType::SEMICOLON, ";", line, column);
            case ',': return Token(TokenType::COMMA, ",", line, column);
            case '(': return Token(TokenType::LPAREN, "(", line, column);
            case ')': return Token(TokenType::RPAREN, ")", line, column);
            case '{': return Token(TokenType::LBRACE, "{", line, column);
            case '}': return Token(TokenType::RBRACE, "}", line, column);
            case '[': return Token(TokenType::LBRACKET, "[", line, column);
            case ']': return Token(TokenType::RBRACKET, "]", line, column);
            default:
                throw std::runtime_error("Unexpected character: " + std::string(1, c));
        }

        return Token(TokenType::END_OF_FILE, "", line, column);
    }
};
