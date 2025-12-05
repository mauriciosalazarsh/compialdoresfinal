#include "../include/semantic.h"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symTable)
    : symbolTable(symTable), currentFunctionReturnType(DataType::VOID), hasErrors(false) {
    FunctionSymbol printlnFunc;
    printlnFunc.name = "println";
    printlnFunc.returnType = DataType::VOID;
    printlnFunc.paramTypes.push_back(DataType::INT);
    symbolTable.declareFunction("println", printlnFunc);

    FunctionSymbol printfFunc;
    printfFunc.name = "printf";
    printfFunc.returnType = DataType::INT;
    printfFunc.paramTypes.push_back(DataType::STRING);
    printfFunc.paramTypes.push_back(DataType::INT);
    symbolTable.declareFunction("printf", printfFunc);
}

void SemanticAnalyzer::error(const std::string& message) {
    errors += "Semantic error: " + message + "\n";
    hasErrors = true;
}

bool SemanticAnalyzer::areTypesCompatible(DataType expected, DataType actual) {
    if (expected == actual) return true;

    // promocion int -> long
    if (expected == DataType::LONG && actual == DataType::INT) return true;

    // promocion int/long -> float
    if (expected == DataType::FLOAT && (actual == DataType::INT || actual == DataType::LONG)) return true;

    // promocion uint -> long
    if (expected == DataType::LONG && actual == DataType::UINT) return true;

    // int -> uint
    if (expected == DataType::UINT && actual == DataType::INT) return true;

    // uint -> int
    if (expected == DataType::INT && actual == DataType::UINT) return true;

    return false;
}

DataType SemanticAnalyzer::getCommonType(DataType t1, DataType t2) {
    if (t1 == t2) return t1;

    if (t1 == DataType::FLOAT || t2 == DataType::FLOAT) return DataType::FLOAT;

    if (t1 == DataType::LONG || t2 == DataType::LONG) return DataType::LONG;

    if ((t1 == DataType::UINT && t2 == DataType::INT) ||
        (t1 == DataType::INT && t2 == DataType::UINT)) {
        return DataType::LONG;
    }

    return t1;
}

void SemanticAnalyzer::visit(BinaryExpr* node) {
    node->left->accept(this);
    node->right->accept(this);

    DataType leftType = node->left->exprType;
    DataType rightType = node->right->exprType;

    // operadores aritmeticos
    if (node->op == "+" || node->op == "-" || node->op == "*" ||
        node->op == "/" || node->op == "%") {
        node->exprType = getCommonType(leftType, rightType);
    }
    // operadores relacionales
    else if (node->op == "<" || node->op == ">" || node->op == "<=" ||
             node->op == ">=" || node->op == "==" || node->op == "!=") {
        node->exprType = DataType::INT;
    }
    // operadores logicos
    else if (node->op == "&&" || node->op == "||") {
        node->exprType = DataType::INT;
    }
}

void SemanticAnalyzer::visit(UnaryExpr* node) {
    node->operand->accept(this);
    node->exprType = node->operand->exprType;
}

void SemanticAnalyzer::visit(TernaryExpr* node) {
    node->condition->accept(this);
    node->trueExpr->accept(this);
    node->falseExpr->accept(this);

    node->exprType = getCommonType(node->trueExpr->exprType, node->falseExpr->exprType);
}

void SemanticAnalyzer::visit(LiteralExpr* node) {
}

void SemanticAnalyzer::visit(IdentifierExpr* node) {
    Symbol* sym = symbolTable.lookup(node->name);
    if (!sym) {
        error("Undefined variable: " + node->name);
        node->exprType = DataType::UNKNOWN;
        return;
    }
    node->exprType = sym->type;
}

void SemanticAnalyzer::visit(ArrayAccessExpr* node) {
    node->array->accept(this);

    for (auto& index : node->indices) {
        index->accept(this);
        if (index->exprType != DataType::INT && index->exprType != DataType::LONG) {
            error("Array index must be of integer type");
        }
    }

    if (auto id = dynamic_cast<IdentifierExpr*>(node->array.get())) {
        Symbol* sym = symbolTable.lookup(id->name);
        if (sym) {
            node->exprType = sym->type;
        }
    }
}

void SemanticAnalyzer::visit(CallExpr* node) {
    FunctionSymbol* func = symbolTable.lookupFunction(node->name);
    if (!func) {
        error("Undefined function: " + node->name);
        node->exprType = DataType::UNKNOWN;
        return;
    }

    bool isVariadic = (node->name == "printf");

    if (!isVariadic && func->paramTypes.size() != node->args.size()) {
        error("Function " + node->name + " expects " +
              std::to_string(func->paramTypes.size()) + " arguments, got " +
              std::to_string(node->args.size()));
    }

    for (size_t i = 0; i < node->args.size(); ++i) {
        node->args[i]->accept(this);
        if (!isVariadic && i < func->paramTypes.size()) {
            if (!areTypesCompatible(func->paramTypes[i], node->args[i]->exprType)) {
                error("Type mismatch in argument " + std::to_string(i + 1) +
                      " of function " + node->name);
            }
        }
    }

    node->exprType = func->returnType;
}

void SemanticAnalyzer::visit(VarDeclStmt* node) {
    if (node->initializer) {
        node->initializer->accept(this);
        if (!areTypesCompatible(node->type, node->initializer->exprType)) {
            error("Type mismatch in variable declaration: " + node->name);
        }
    }

    Symbol sym;
    sym.name = node->name;
    sym.type = node->type;
    sym.isMutable = node->isMutable;
    sym.arrayDimensions = node->arrayDimensions;

    int size = 8;
    if (!node->arrayDimensions.empty()) {
        for (int dim : node->arrayDimensions) {
            if (dim > 0) size *= dim;
        }
    }
    sym.offset = symbolTable.allocateStackSpace(size);

    if (!symbolTable.declareVariable(node->name, sym)) {
        error("Variable already declared: " + node->name);
    }
}

void SemanticAnalyzer::visit(AssignStmt* node) {
    node->target->accept(this);
    node->value->accept(this);

    if (!node->target->isLValue) {
        error("Left side of assignment must be an lvalue");
    }

    if (!areTypesCompatible(node->target->exprType, node->value->exprType)) {
        error("Type mismatch in assignment");
    }
}

void SemanticAnalyzer::visit(ExprStmt* node) {
    node->expression->accept(this);
}

void SemanticAnalyzer::visit(IfStmt* node) {
    node->condition->accept(this);
    node->thenBranch->accept(this);
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
}

void SemanticAnalyzer::visit(WhileStmt* node) {
    node->condition->accept(this);
    node->body->accept(this);
}

void SemanticAnalyzer::visit(ForStmt* node) {
    symbolTable.enterScope();

    Symbol loopVar;
    loopVar.name = node->varName;
    loopVar.type = DataType::INT;
    loopVar.isMutable = false;
    loopVar.offset = symbolTable.allocateStackSpace(8);
    symbolTable.declareVariable(node->varName, loopVar);

    node->start->accept(this);
    node->end->accept(this);
    node->body->accept(this);

    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(BlockStmt* node) {
    symbolTable.enterScope();
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(ReturnStmt* node) {
    if (node->value) {
        node->value->accept(this);
        if (!areTypesCompatible(currentFunctionReturnType, node->value->exprType)) {
            error("Return type mismatch");
        }
    } else if (currentFunctionReturnType != DataType::VOID) {
        error("Function must return a value");
    }
}

void SemanticAnalyzer::visit(FunctionDecl* node) {
    FunctionSymbol funcSym;
    funcSym.name = node->name;
    funcSym.returnType = node->returnType;

    for (auto& param : node->params) {
        funcSym.paramTypes.push_back(param.type);
        funcSym.paramNames.push_back(param.name);
    }

    if (!symbolTable.declareFunction(node->name, funcSym)) {
        error("Function already declared: " + node->name);
    }

    symbolTable.enterScope();
    symbolTable.resetOffset();
    currentFunctionReturnType = node->returnType;

    int paramOffset = 16;
    for (auto& param : node->params) {
        Symbol paramSym;
        paramSym.name = param.name;
        paramSym.type = param.type;
        paramSym.isMutable = true;
        paramSym.isParameter = true;
        paramSym.offset = paramOffset;
        paramSym.arrayDimensions = param.arrayDimensions;
        paramOffset += 8;

        symbolTable.declareVariable(param.name, paramSym);
    }

    node->body->accept(this);

    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(Program* node) {
    for (auto& func : node->functions) {
        func->accept(this);
    }

    if (!symbolTable.lookupFunction("main")) {
        error("No main function defined");
    }
}
