#include "../include/semantic.h"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symTable)
    : symbolTable(symTable), currentFunctionReturnType(DataType::VOID), hasErrors(false) {
    // Register built-in functions
    FunctionSymbol printlnFunc;
    printlnFunc.name = "println";
    printlnFunc.returnType = DataType::VOID;
    printlnFunc.paramTypes.push_back(DataType::INT); // Accept any int-like value
    symbolTable.declareFunction("println", printlnFunc);

    // Register printf for C compatibility
    FunctionSymbol printfFunc;
    printfFunc.name = "printf";
    printfFunc.returnType = DataType::INT;
    printfFunc.paramTypes.push_back(DataType::STRING); // Format string
    printfFunc.paramTypes.push_back(DataType::INT);    // Variable args (simplified)
    symbolTable.declareFunction("printf", printfFunc);
}

void SemanticAnalyzer::error(const std::string& message) {
    errors += "Semantic error: " + message + "\n";
    hasErrors = true;
}

bool SemanticAnalyzer::areTypesCompatible(DataType expected, DataType actual) {
    if (expected == actual) return true;

    // Allow INT -> LONG promotion
    if (expected == DataType::LONG && actual == DataType::INT) return true;

    // Allow INT/LONG -> FLOAT promotion
    if (expected == DataType::FLOAT && (actual == DataType::INT || actual == DataType::LONG)) return true;

    // Allow UINT -> LONG promotion
    if (expected == DataType::LONG && actual == DataType::UINT) return true;

    // Allow INT -> UINT (for positive literals)
    if (expected == DataType::UINT && actual == DataType::INT) return true;

    // Allow UINT -> INT
    if (expected == DataType::INT && actual == DataType::UINT) return true;

    return false;
}

DataType SemanticAnalyzer::getCommonType(DataType t1, DataType t2) {
    if (t1 == t2) return t1;

    // Float dominates
    if (t1 == DataType::FLOAT || t2 == DataType::FLOAT) return DataType::FLOAT;

    // Long dominates over Int
    if (t1 == DataType::LONG || t2 == DataType::LONG) return DataType::LONG;

    // UINT with INT/LONG
    if ((t1 == DataType::UINT && t2 == DataType::INT) ||
        (t1 == DataType::INT && t2 == DataType::UINT)) {
        return DataType::LONG; // Promote to signed long
    }

    return t1;
}

void SemanticAnalyzer::visit(BinaryExpr* node) {
    node->left->accept(this);
    node->right->accept(this);

    DataType leftType = node->left->exprType;
    DataType rightType = node->right->exprType;

    // Arithmetic operators
    if (node->op == "+" || node->op == "-" || node->op == "*" ||
        node->op == "/" || node->op == "%") {
        node->exprType = getCommonType(leftType, rightType);
    }
    // Relational operators
    else if (node->op == "<" || node->op == ">" || node->op == "<=" ||
             node->op == ">=" || node->op == "==" || node->op == "!=") {
        node->exprType = DataType::INT; // Boolean result (represented as int)
    }
    // Logical operators
    else if (node->op == "&&" || node->op == "||") {
        node->exprType = DataType::INT; // Boolean result
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
    // Type already set in constructor
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

    // For arrays, the type is the element type
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

    // printf is variadic, skip strict argument count check
    bool isVariadic = (node->name == "printf");

    if (!isVariadic && func->paramTypes.size() != node->args.size()) {
        error("Function " + node->name + " expects " +
              std::to_string(func->paramTypes.size()) + " arguments, got " +
              std::to_string(node->args.size()));
    }

    for (size_t i = 0; i < node->args.size(); ++i) {
        node->args[i]->accept(this);
        // For variadic functions, only check first argument (format string)
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

    // Calculate size for stack allocation
    int size = 8; // Base size for all types in x86-64
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

    // Declare loop variable
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
    // Register function in symbol table
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

    // Enter function scope
    symbolTable.enterScope();
    symbolTable.resetOffset();
    currentFunctionReturnType = node->returnType;

    // Declare parameters
    int paramOffset = 16; // Start after saved rbp and return address
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

    // Analyze function body
    node->body->accept(this);

    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(Program* node) {
    for (auto& func : node->functions) {
        func->accept(this);
    }

    // Check that main function exists
    if (!symbolTable.lookupFunction("main")) {
        error("No main function defined");
    }
}
