#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "visitor.h"
#include "symboltable.h"
#include "ast.h"
#include <string>

class SemanticAnalyzer : public Visitor {
private:
    SymbolTable& symbolTable;
    DataType currentFunctionReturnType;
    std::string errors;
    bool hasErrors;

    void error(const std::string& message);
    bool areTypesCompatible(DataType expected, DataType actual);
    DataType getCommonType(DataType t1, DataType t2);

public:
    SemanticAnalyzer(SymbolTable& symTable);

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

    bool hasError() const { return hasErrors; }
    std::string getErrors() const { return errors; }
};

#endif // SEMANTIC_H
