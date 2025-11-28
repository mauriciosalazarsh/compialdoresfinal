#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class Visitor;

// Data types
enum class DataType {
    INT, LONG, UINT, FLOAT, STRING, VOID, ARRAY, UNKNOWN
};

std::string dataTypeToString(DataType type);

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(Visitor* visitor) = 0;
};

// Expressions
class Expr : public ASTNode {
public:
    DataType exprType = DataType::UNKNOWN;
    bool isLValue = false;
};

class BinaryExpr : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> l, std::string o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    void accept(Visitor* visitor) override;
};

class UnaryExpr : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(std::string o, std::unique_ptr<Expr> expr)
        : op(o), operand(std::move(expr)) {}
    void accept(Visitor* visitor) override;
};

class TernaryExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueExpr;
    std::unique_ptr<Expr> falseExpr;

    TernaryExpr(std::unique_ptr<Expr> cond, std::unique_ptr<Expr> t, std::unique_ptr<Expr> f)
        : condition(std::move(cond)), trueExpr(std::move(t)), falseExpr(std::move(f)) {}
    void accept(Visitor* visitor) override;
};

class LiteralExpr : public Expr {
public:
    std::string value;

    LiteralExpr(const std::string& val, DataType type) : value(val) {
        exprType = type;
    }
    void accept(Visitor* visitor) override;
};

class IdentifierExpr : public Expr {
public:
    std::string name;

    IdentifierExpr(const std::string& n) : name(n) {
        isLValue = true;
    }
    void accept(Visitor* visitor) override;
};

class ArrayAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> array;
    std::vector<std::unique_ptr<Expr>> indices;

    ArrayAccessExpr(std::unique_ptr<Expr> arr, std::vector<std::unique_ptr<Expr>> idx)
        : array(std::move(arr)), indices(std::move(idx)) {
        isLValue = true;
    }
    void accept(Visitor* visitor) override;
};

class CallExpr : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    CallExpr(const std::string& n, std::vector<std::unique_ptr<Expr>> a)
        : name(n), args(std::move(a)) {}
    void accept(Visitor* visitor) override;
};

// Statements
class Stmt : public ASTNode {};

class VarDeclStmt : public Stmt {
public:
    bool isMutable; // var vs val
    std::string name;
    DataType type;
    std::unique_ptr<Expr> initializer;
    std::vector<int> arrayDimensions; // For multidimensional arrays

    VarDeclStmt(bool mut, const std::string& n, DataType t, std::unique_ptr<Expr> init)
        : isMutable(mut), name(n), type(t), initializer(std::move(init)) {}
    void accept(Visitor* visitor) override;
};

class AssignStmt : public Stmt {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignStmt(std::unique_ptr<Expr> t, std::unique_ptr<Expr> v)
        : target(std::move(t)), value(std::move(v)) {}
    void accept(Visitor* visitor) override;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
    void accept(Visitor* visitor) override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, std::unique_ptr<Stmt> els = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(then)), elseBranch(std::move(els)) {}
    void accept(Visitor* visitor) override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

class ForStmt : public Stmt {
public:
    std::string varName;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Stmt> body;

    ForStmt(const std::string& var, std::unique_ptr<Expr> s, std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b)
        : varName(var), start(std::move(s)), end(std::move(e)), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : statements(std::move(stmts)) {}
    void accept(Visitor* visitor) override;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;

    ReturnStmt(std::unique_ptr<Expr> v = nullptr) : value(std::move(v)) {}
    void accept(Visitor* visitor) override;
};

// Function Parameter
struct Parameter {
    std::string name;
    DataType type;
    std::vector<int> arrayDimensions; // For array parameters
};

// Function Declaration
class FunctionDecl : public ASTNode {
public:
    std::string name;
    std::vector<Parameter> params;
    DataType returnType;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(const std::string& n, std::vector<Parameter> p, DataType ret, std::unique_ptr<BlockStmt> b)
        : name(n), params(std::move(p)), returnType(ret), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

// Program (root node)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    Program(std::vector<std::unique_ptr<FunctionDecl>> funcs) : functions(std::move(funcs)) {}
    void accept(Visitor* visitor) override;
};

#endif // AST_H
