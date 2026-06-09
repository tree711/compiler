#pragma once

#include <string>
#include <unordered_map>
#include <vector>

enum class SymbolType {
    INTEGER
};

struct Symbol {
    std::string name;
    SymbolType type;
    int scope;
    int line;
    int column;
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;

public:
    SymbolTable();

    void enterScope();
    void exitScope();
    bool declare(const std::string& name, SymbolType type, int line, int column);
    const Symbol* lookup(const std::string& name) const;
    bool existsInCurrentScope(const std::string& name) const;
    int currentScope() const;
};
