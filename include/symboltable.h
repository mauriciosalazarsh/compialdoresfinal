#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "ast.h"
#include <string>
#include <map>
#include <vector>

struct Symbol {
    std::string name;
    DataType type;
    bool isMutable;
    int offset; // Stack offset for local variables
    bool isParameter;
    std::vector<int> arrayDimensions; // For arrays

    Symbol() : type(DataType::UNKNOWN), isMutable(false), offset(0), isParameter(false) {}
};

struct FunctionSymbol {
    std::string name;
    DataType returnType;
    std::vector<DataType> paramTypes;
    std::vector<std::string> paramNames;
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol>> scopes;
    std::map<std::string, FunctionSymbol> functions;
    int currentOffset;

public:
    SymbolTable();
    void enterScope();
    void exitScope();

    bool declareVariable(const std::string& name, const Symbol& symbol);
    Symbol* lookup(const std::string& name);
    bool declareFunction(const std::string& name, const FunctionSymbol& func);
    FunctionSymbol* lookupFunction(const std::string& name);

    int allocateStackSpace(int size);
    int getCurrentOffset() const { return currentOffset; }
    void resetOffset() { currentOffset = 0; }
};

#endif // SYMBOLTABLE_H
