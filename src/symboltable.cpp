#include "../include/symboltable.h"

SymbolTable::SymbolTable() : currentOffset(0) {
    enterScope(); // Global scope
}

void SymbolTable::enterScope() {
    scopes.push_back(std::map<std::string, Symbol>());
}

void SymbolTable::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

bool SymbolTable::declareVariable(const std::string& name, const Symbol& symbol) {
    if (scopes.empty()) return false;

    auto& currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
        return false; // Already declared in this scope
    }

    currentScope[name] = symbol;
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::declareFunction(const std::string& name, const FunctionSymbol& func) {
    if (functions.find(name) != functions.end()) {
        return false; // Already declared
    }
    functions[name] = func;
    return true;
}

FunctionSymbol* SymbolTable::lookupFunction(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    return nullptr;
}

int SymbolTable::allocateStackSpace(int size) {
    currentOffset -= size;
    return currentOffset;
}
