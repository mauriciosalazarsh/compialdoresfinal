#ifndef VISITOR_H
#define VISITOR_H

class BinaryExpr;
class UnaryExpr;
class TernaryExpr;
class LiteralExpr;
class IdentifierExpr;
class ArrayAccessExpr;
class CallExpr;
class VarDeclStmt;
class AssignStmt;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class BlockStmt;
class ReturnStmt;
class FunctionDecl;
class Program;

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(BinaryExpr* node) = 0;
    virtual void visit(UnaryExpr* node) = 0;
    virtual void visit(TernaryExpr* node) = 0;
    virtual void visit(LiteralExpr* node) = 0;
    virtual void visit(IdentifierExpr* node) = 0;
    virtual void visit(ArrayAccessExpr* node) = 0;
    virtual void visit(CallExpr* node) = 0;
    virtual void visit(VarDeclStmt* node) = 0;
    virtual void visit(AssignStmt* node) = 0;
    virtual void visit(ExprStmt* node) = 0;
    virtual void visit(IfStmt* node) = 0;
    virtual void visit(WhileStmt* node) = 0;
    virtual void visit(ForStmt* node) = 0;
    virtual void visit(BlockStmt* node) = 0;
    virtual void visit(ReturnStmt* node) = 0;
    virtual void visit(FunctionDecl* node) = 0;
    virtual void visit(Program* node) = 0;
};

#endif // VISITOR_H
