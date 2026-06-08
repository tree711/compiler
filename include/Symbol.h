#pragma once

#include <string>
#include <map>
#include <vector>

enum class SymbolType {
    INTEGER,
    FLOAT,
    VOID,
    FUNCTION
};

struct Symbol {
    std::string name;
    SymbolType type;
    int scope;
    int line;
    
    Symbol(const std::string& n, SymbolType t, int s, int l)
        : name(n), type(t), scope(s), line(l) {}
};

struct FunctionSymbol : public Symbol {
    SymbolType returnType;
    std::vector<SymbolType> parameters;
    
    FunctionSymbol(const std::string& n, SymbolType rt, const std::vector<SymbolType>& params, int s, int l)
        : Symbol(n, SymbolType::FUNCTION, s, l), returnType(rt), parameters(params) {}
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol*>> scopes;
    int currentScope;

public:
    SymbolTable() : currentScope(0) {
        scopes.push_back({});
    }

    ~SymbolTable() {
        for (auto& scope : scopes) {
            for (auto& entry : scope) {
                delete entry.second;
            }
        }
    }

    void enterScope() {
        currentScope++;
        scopes.push_back({});
    }

    void exitScope() {
        if (currentScope > 0) {
            for (auto& entry : scopes[currentScope]) {
                delete entry.second;
            }
            scopes.pop_back();
            currentScope--;
        }
    }

    bool insert(Symbol* symbol) {
        if (scopes[currentScope].find(symbol->name) != scopes[currentScope].end()) {
            return false;
        }
        scopes[currentScope][symbol->name] = symbol;
        return true;
    }

    Symbol* lookup(const std::string& name) {
        for (int i = currentScope; i >= 0; i--) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    bool existsInCurrentScope(const std::string& name) const {
        return scopes[currentScope].find(name) != scopes[currentScope].end();
    }

    int getCurrentScope() const {
        return currentScope;
    }
};
