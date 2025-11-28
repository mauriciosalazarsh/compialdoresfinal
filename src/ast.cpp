#include "../include/ast.h"
#include "../include/visitor.h"

std::string dataTypeToString(DataType type) {
    switch(type) {
        case DataType::INT: return "Int";
        case DataType::LONG: return "Long";
        case DataType::UINT: return "UInt";
        case DataType::FLOAT: return "Float";
        case DataType::STRING: return "String";
        case DataType::VOID: return "Void";
        case DataType::ARRAY: return "Array";
        case DataType::UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

// Accept methods for all node types
void BinaryExpr::accept(Visitor* visitor) { visitor->visit(this); }
void UnaryExpr::accept(Visitor* visitor) { visitor->visit(this); }
void TernaryExpr::accept(Visitor* visitor) { visitor->visit(this); }
void LiteralExpr::accept(Visitor* visitor) { visitor->visit(this); }
void IdentifierExpr::accept(Visitor* visitor) { visitor->visit(this); }
void ArrayAccessExpr::accept(Visitor* visitor) { visitor->visit(this); }
void CallExpr::accept(Visitor* visitor) { visitor->visit(this); }
void VarDeclStmt::accept(Visitor* visitor) { visitor->visit(this); }
void AssignStmt::accept(Visitor* visitor) { visitor->visit(this); }
void ExprStmt::accept(Visitor* visitor) { visitor->visit(this); }
void IfStmt::accept(Visitor* visitor) { visitor->visit(this); }
void WhileStmt::accept(Visitor* visitor) { visitor->visit(this); }
void ForStmt::accept(Visitor* visitor) { visitor->visit(this); }
void BlockStmt::accept(Visitor* visitor) { visitor->visit(this); }
void ReturnStmt::accept(Visitor* visitor) { visitor->visit(this); }
void FunctionDecl::accept(Visitor* visitor) { visitor->visit(this); }
void Program::accept(Visitor* visitor) { visitor->visit(this); }
