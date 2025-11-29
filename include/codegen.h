#ifndef CODEGEN_H
#define CODEGEN_H

#include "visitor.h"
#include "symboltable.h"
#include "ast.h"
#include <string>
#include <sstream>
#include <stack>

class CodeGenerator : public Visitor {
private:
    SymbolTable& symbolTable;
    std::ostringstream code;
    std::ostringstream dataSection;
    int labelCounter;
    int stringCounter;
    std::string currentFunction;

    bool enableConstantFolding;
    bool enableDeadCodeElimination;

    std::string newLabel();
    std::string newStringLabel();
    void emit(const std::string& instruction);
    void emitLabel(const std::string& label);
    void generatePrologue(const std::string& funcName, int stackSize);
    void generateEpilogue();
    void loadVariable(const std::string& varName, DataType type);
    void storeVariable(const std::string& varName, DataType type);
    void convertType(DataType from, DataType to);
    void computeArrayOffset(const std::vector<std::unique_ptr<Expr>>& indices,
                           const std::vector<int>& dimensions);
    void preDeclareVariables(Stmt* stmt);
    std::string escapeString(const std::string& str);

public:
    CodeGenerator(SymbolTable& symTable);

    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(TernaryExpr* node) override;
    void visit(LiteralExpr* node) override;
    void visit(IdentifierExpr* node) override;
    void visit(ArrayAccessExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(AssignStmt* node) override;
    void visit(ExprStmt* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(ForStmt* node) override;
    void visit(BlockStmt* node) override;
    void visit(ReturnStmt* node) override;
    void visit(FunctionDecl* node) override;
    void visit(Program* node) override;

    std::string getCode() const;
    void enableOptimizations(bool constantFold, bool deadCode);
};

#endif // CODEGEN_H
