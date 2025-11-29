#include "../include/codegen.h"
#include <iostream>

CodeGenerator::CodeGenerator(SymbolTable& symTable)
    : symbolTable(symTable), labelCounter(0), stringCounter(0),
      enableConstantFolding(true), enableDeadCodeElimination(true) {}

std::string CodeGenerator::newLabel() {
    return ".L" + std::to_string(labelCounter++);
}

std::string CodeGenerator::newStringLabel() {
    return ".STR" + std::to_string(stringCounter++);
}

void CodeGenerator::emit(const std::string& instruction) {
    code << "    " << instruction << "\n";
}

void CodeGenerator::emitLabel(const std::string& label) {
    code << label << ":\n";
}

void CodeGenerator::generatePrologue(const std::string& funcName, int stackSize) {
    emitLabel(funcName);
    emit("push rbp");
    emit("mov rbp, rsp");
    // alinear a 16 bytes
    int alignedSize = ((stackSize + 15) & ~15);
    if (alignedSize > 0) {
        emit("sub rsp, " + std::to_string(alignedSize));
    }
}

void CodeGenerator::generateEpilogue() {
    emit("mov rsp, rbp");
    emit("pop rbp");
    emit("ret");
}

void CodeGenerator::loadVariable(const std::string& varName, DataType type) {
    Symbol* sym = symbolTable.lookup(varName);
    if (!sym) return;

    if (sym->isParameter) {
        emit("mov rax, [rbp + " + std::to_string(sym->offset) + "]");
    } else {
        emit("mov rax, [rbp - " + std::to_string(-sym->offset) + "]");
    }

    if (type == DataType::FLOAT) {
        emit("movq xmm0, rax");
    }
}

void CodeGenerator::storeVariable(const std::string& varName, DataType type) {
    Symbol* sym = symbolTable.lookup(varName);
    if (!sym) return;

    if (type == DataType::FLOAT) {
        emit("movq rax, xmm0");
    }

    if (sym->isParameter) {
        emit("mov [rbp + " + std::to_string(sym->offset) + "], rax");
    } else {
        emit("mov [rbp - " + std::to_string(-sym->offset) + "], rax");
    }
}

void CodeGenerator::convertType(DataType from, DataType to) {
    if (from == to) return;

    // int/long/uint -> float
    if ((from == DataType::INT || from == DataType::LONG || from == DataType::UINT) &&
        to == DataType::FLOAT) {
        emit("cvtsi2sd xmm0, rax");
    }
    // float -> int/long
    else if (from == DataType::FLOAT &&
             (to == DataType::INT || to == DataType::LONG || to == DataType::UINT)) {
        emit("cvttsd2si rax, xmm0");
    }
    // int -> long
    else if (from == DataType::INT && to == DataType::LONG) {
        emit("cdqe");
    }
    // uint -> long
    else if (from == DataType::UINT && to == DataType::LONG) {
        emit("mov eax, eax");
    }
}

void CodeGenerator::computeArrayOffset(const std::vector<std::unique_ptr<Expr>>& indices,
                                      const std::vector<int>& dimensions) {
    if (indices.empty()) return;

    indices[0]->accept(this);
    emit("push rax");

    for (size_t i = 1; i < indices.size(); ++i) {
        int dimProduct = 1;
        for (size_t j = i; j < dimensions.size(); ++j) {
            dimProduct *= dimensions[j];
        }

        emit("pop rax"); // Get previous result
        emit("imul rax, " + std::to_string(dimProduct));
        emit("push rax");

        indices[i]->accept(this);
        emit("pop rbx");
        emit("add rax, rbx");
        emit("push rax");
    }

    emit("pop rax");
    emit("imul rax, 8");
}

void CodeGenerator::visit(BinaryExpr* node) {
    // constant folding
    if (enableConstantFolding) {
        auto leftLit = dynamic_cast<LiteralExpr*>(node->left.get());
        auto rightLit = dynamic_cast<LiteralExpr*>(node->right.get());

        if (leftLit && rightLit && node->exprType != DataType::FLOAT) {
            long long leftVal = std::stoll(leftLit->value);
            long long rightVal = std::stoll(rightLit->value);
            long long result = 0;

            if (node->op == "+") result = leftVal + rightVal;
            else if (node->op == "-") result = leftVal - rightVal;
            else if (node->op == "*") result = leftVal * rightVal;
            else if (node->op == "/" && rightVal != 0) result = leftVal / rightVal;
            else if (node->op == "%" && rightVal != 0) result = leftVal % rightVal;
            else goto no_fold;

            emit("mov rax, " + std::to_string(result));
            return;
        }
    }

no_fold:
    node->left->accept(this);
    emit("push rax");

    node->right->accept(this);
    emit("mov rbx, rax");
    emit("pop rax");

    // operaciones float
    if (node->exprType == DataType::FLOAT) {
        emit("movq xmm0, rax");
        emit("movq xmm1, rbx");

        if (node->op == "+") emit("addsd xmm0, xmm1");
        else if (node->op == "-") emit("subsd xmm0, xmm1");
        else if (node->op == "*") emit("mulsd xmm0, xmm1");
        else if (node->op == "/") emit("divsd xmm0, xmm1");

        emit("movq rax, xmm0");
    } else {
        // aritmetica entera
        if (node->op == "+") {
            emit("add rax, rbx");
        } else if (node->op == "-") {
            emit("sub rax, rbx");
        } else if (node->op == "*") {
            emit("imul rax, rbx");
        } else if (node->op == "/") {
            emit("cqo");
            emit("idiv rbx");
        } else if (node->op == "%") {
            emit("cqo");
            emit("idiv rbx");
            emit("mov rax, rdx");
        }
        // comparaciones
        else if (node->op == "<") {
            emit("cmp rax, rbx");
            emit("setl al");
            emit("movzx rax, al");
        } else if (node->op == "<=") {
            emit("cmp rax, rbx");
            emit("setle al");
            emit("movzx rax, al");
        } else if (node->op == ">") {
            emit("cmp rax, rbx");
            emit("setg al");
            emit("movzx rax, al");
        } else if (node->op == ">=") {
            emit("cmp rax, rbx");
            emit("setge al");
            emit("movzx rax, al");
        } else if (node->op == "==") {
            emit("cmp rax, rbx");
            emit("sete al");
            emit("movzx rax, al");
        } else if (node->op == "!=") {
            emit("cmp rax, rbx");
            emit("setne al");
            emit("movzx rax, al");
        }
        // logicas
        else if (node->op == "&&") {
            emit("and rax, rbx");
        } else if (node->op == "||") {
            emit("or rax, rbx");
        }
    }
}

void CodeGenerator::visit(UnaryExpr* node) {
    node->operand->accept(this);

    if (node->op == "-") {
        if (node->exprType == DataType::FLOAT) {
            emit("movq xmm0, rax");
            emit("xorpd xmm1, xmm1");
            emit("subsd xmm1, xmm0");
            emit("movq rax, xmm1");
        } else {
            emit("neg rax");
        }
    } else if (node->op == "!") {
        emit("test rax, rax");
        emit("setz al");
        emit("movzx rax, al");
    }
}

void CodeGenerator::visit(TernaryExpr* node) {
    std::string falseLabel = newLabel();
    std::string endLabel = newLabel();

    node->condition->accept(this);
    emit("test rax, rax");
    emit("jz " + falseLabel);

    node->trueExpr->accept(this);
    emit("jmp " + endLabel);

    emitLabel(falseLabel);
    node->falseExpr->accept(this);

    emitLabel(endLabel);
}

std::string CodeGenerator::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            default: result += c; break;
        }
    }
    return result;
}

void CodeGenerator::visit(LiteralExpr* node) {
    if (node->exprType == DataType::FLOAT) {
        std::string label = newStringLabel();
        dataSection << label << ": .double " << node->value << "\n";
        emit("movsd xmm0, [" + label + "]");
        emit("movq rax, xmm0");
    } else if (node->exprType == DataType::STRING) {
        std::string label = newStringLabel();
        std::string escaped = escapeString(node->value);
        dataSection << label << ": .asciz \"" << escaped << "\"\n";
        emit("lea rax, [" + label + "]");
    } else {
        emit("mov rax, " + node->value);
    }
}

void CodeGenerator::visit(IdentifierExpr* node) {
    Symbol* sym = symbolTable.lookup(node->name);
    if (sym) {
        loadVariable(node->name, sym->type);
    }
}

void CodeGenerator::visit(ArrayAccessExpr* node) {
    auto idExpr = dynamic_cast<IdentifierExpr*>(node->array.get());
    if (!idExpr) return;

    Symbol* sym = symbolTable.lookup(idExpr->name);
    if (!sym) return;

    computeArrayOffset(node->indices, sym->arrayDimensions);

    if (sym->isParameter) {
        emit("mov rbx, [rbp + " + std::to_string(sym->offset) + "]");
    } else {
        emit("lea rbx, [rbp - " + std::to_string(-sym->offset) + "]");
    }

    emit("add rbx, rax");
    emit("mov rax, [rbx]");
}

void CodeGenerator::visit(CallExpr* node) {
    // println
    if (node->name == "println") {
        if (!node->args.empty()) {
            node->args[0]->accept(this);
            emit("mov rsi, rax");
            emit("lea rdi, [int_fmt]");
            emit("xor eax, eax");
            emit("sub rsp, 8");
            emit("call printf");
            emit("add rsp, 8");
        }
        return;
    }

    // printf
    if (node->name == "printf") {
        std::vector<std::string> argRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        int numArgs = node->args.size();

        bool hasFloatArg = false;
        for (size_t i = 1; i < node->args.size(); ++i) {
            if (node->args[i]->exprType == DataType::FLOAT) {
                hasFloatArg = true;
                break;
            }
        }

        if (hasFloatArg && numArgs >= 2) {
            node->args[0]->accept(this);
            emit("mov rdi, rax");

            node->args[1]->accept(this);
            emit("movq xmm0, rax");

            emit("mov eax, 1");
            emit("call printf");
            return;
        }

        for (int i = numArgs - 1; i >= 0; --i) {
            node->args[i]->accept(this);
            emit("push rax");
        }

        for (int i = 0; i < numArgs && i < (int)argRegs.size(); ++i) {
            emit("pop " + argRegs[i]);
        }

        emit("xor eax, eax");
        emit("call printf");
        return;
    }

    // funciones definidas por usuario
    int numArgs = node->args.size();
    int padding = (numArgs % 2 == 1) ? 8 : 0;

    if (padding > 0) {
        emit("sub rsp, " + std::to_string(padding));
    }

    for (int i = numArgs - 1; i >= 0; --i) {
        node->args[i]->accept(this);
        emit("push rax");
    }

    emit("call " + node->name);

    int cleanup = numArgs * 8 + padding;
    if (cleanup > 0) {
        emit("add rsp, " + std::to_string(cleanup));
    }
}

void CodeGenerator::visit(VarDeclStmt* node) {
    Symbol* sym = symbolTable.lookup(node->name);
    if (!sym) {
        Symbol newSym;
        newSym.name = node->name;
        newSym.type = node->type;
        newSym.isParameter = false;

        int arraySize = 8;
        if (!node->arrayDimensions.empty()) {
            arraySize = 8;
            for (int dim : node->arrayDimensions) {
                arraySize *= dim;
            }
        }

        newSym.offset = symbolTable.allocateStackSpace(arraySize);
        newSym.arrayDimensions = node->arrayDimensions;
        symbolTable.declareVariable(node->name, newSym);
        sym = symbolTable.lookup(node->name);
    }

    if (node->initializer) {
        node->initializer->accept(this);
        convertType(node->initializer->exprType, node->type);
        storeVariable(node->name, node->type);
    }
}

void CodeGenerator::visit(AssignStmt* node) {
    node->value->accept(this);
    emit("push rax");

    if (auto arrayAccess = dynamic_cast<ArrayAccessExpr*>(node->target.get())) {
        auto idExpr = dynamic_cast<IdentifierExpr*>(arrayAccess->array.get());
        if (idExpr) {
            Symbol* sym = symbolTable.lookup(idExpr->name);
            if (sym) {
                computeArrayOffset(arrayAccess->indices, sym->arrayDimensions);

                if (sym->isParameter) {
                    emit("mov rbx, [rbp + " + std::to_string(sym->offset) + "]");
                } else {
                    emit("lea rbx, [rbp - " + std::to_string(-sym->offset) + "]");
                }

                emit("add rbx, rax");
                emit("pop rax");
                emit("mov [rbx], rax");
            }
        }
    } else if (auto id = dynamic_cast<IdentifierExpr*>(node->target.get())) {
        emit("pop rax");
        Symbol* sym = symbolTable.lookup(id->name);
        if (sym) {
            storeVariable(id->name, sym->type);
        }
    }
}

void CodeGenerator::visit(ExprStmt* node) {
    node->expression->accept(this);
}

void CodeGenerator::visit(IfStmt* node) {
    std::string elseLabel = newLabel();
    std::string endLabel = newLabel();

    node->condition->accept(this);
    emit("test rax, rax");

    if (node->elseBranch) {
        emit("jz " + elseLabel);
        node->thenBranch->accept(this);
        emit("jmp " + endLabel);
        emitLabel(elseLabel);
        node->elseBranch->accept(this);
        emitLabel(endLabel);
    } else {
        emit("jz " + endLabel);
        node->thenBranch->accept(this);
        emitLabel(endLabel);
    }
}

void CodeGenerator::visit(WhileStmt* node) {
    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    emitLabel(startLabel);
    node->condition->accept(this);
    emit("test rax, rax");
    emit("jz " + endLabel);

    node->body->accept(this);
    emit("jmp " + startLabel);

    emitLabel(endLabel);
}

void CodeGenerator::visit(ForStmt* node) {
    std::string startLabel = newLabel();
    std::string endLabel = newLabel();

    node->start->accept(this);
    Symbol* loopVar = symbolTable.lookup(node->varName);
    if (loopVar) {
        storeVariable(node->varName, DataType::INT);
    }

    emitLabel(startLabel);

    loadVariable(node->varName, DataType::INT);
    emit("push rax");
    node->end->accept(this);
    emit("pop rbx");
    emit("cmp rbx, rax");
    emit("jge " + endLabel);

    node->body->accept(this);

    loadVariable(node->varName, DataType::INT);
    emit("inc rax");
    storeVariable(node->varName, DataType::INT);

    emit("jmp " + startLabel);
    emitLabel(endLabel);
}

void CodeGenerator::visit(BlockStmt* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void CodeGenerator::visit(ReturnStmt* node) {
    if (node->value) {
        node->value->accept(this);
    }
    generateEpilogue();
}

void CodeGenerator::preDeclareVariables(Stmt* stmt) {
    if (auto varDecl = dynamic_cast<VarDeclStmt*>(stmt)) {
        Symbol* existing = symbolTable.lookup(varDecl->name);
        if (!existing) {
            Symbol newSym;
            newSym.name = varDecl->name;
            newSym.type = varDecl->type;
            newSym.isParameter = false;

            int arraySize = 8;
            if (!varDecl->arrayDimensions.empty()) {
                arraySize = 8;
                for (int dim : varDecl->arrayDimensions) {
                    arraySize *= dim;
                }
            }

            newSym.offset = symbolTable.allocateStackSpace(arraySize);
            newSym.arrayDimensions = varDecl->arrayDimensions;
            symbolTable.declareVariable(varDecl->name, newSym);
        }
    } else if (auto block = dynamic_cast<BlockStmt*>(stmt)) {
        for (auto& s : block->statements) {
            preDeclareVariables(s.get());
        }
    } else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        preDeclareVariables(ifStmt->thenBranch.get());
        if (ifStmt->elseBranch) {
            preDeclareVariables(ifStmt->elseBranch.get());
        }
    } else if (auto whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        preDeclareVariables(whileStmt->body.get());
    } else if (auto forStmt = dynamic_cast<ForStmt*>(stmt)) {
        Symbol* existing = symbolTable.lookup(forStmt->varName);
        if (!existing) {
            Symbol newSym;
            newSym.name = forStmt->varName;
            newSym.type = DataType::INT;
            newSym.isParameter = false;
            newSym.offset = symbolTable.allocateStackSpace(8);
            symbolTable.declareVariable(forStmt->varName, newSym);
        }
        preDeclareVariables(forStmt->body.get());
    }
}

void CodeGenerator::visit(FunctionDecl* node) {
    currentFunction = node->name;

    symbolTable.enterScope();
    symbolTable.resetOffset();

    int paramOffset = 16;
    for (auto& param : node->params) {
        Symbol paramSym;
        paramSym.name = param.name;
        paramSym.type = param.type;
        paramSym.isParameter = true;
        paramSym.offset = paramOffset;
        paramSym.arrayDimensions = param.arrayDimensions;
        paramOffset += 8;
        symbolTable.declareVariable(param.name, paramSym);
    }

    preDeclareVariables(node->body.get());
    int stackSize = -symbolTable.getCurrentOffset();

    generatePrologue(node->name, stackSize);
    node->body->accept(this);

    if (node->returnType == DataType::VOID) {
        generateEpilogue();
    }

    symbolTable.exitScope();
}

void CodeGenerator::visit(Program* node) {
    code << ".intel_syntax noprefix\n";
    code << ".text\n";
    code << ".global main\n\n";

    for (auto& func : node->functions) {
        func->accept(this);
        code << "\n";
    }

    code << "print_int:\n";
    code << "    push rbp\n";
    code << "    mov rbp, rsp\n";
    code << "    mov rsi, rdi\n";
    code << "    lea rdi, [int_fmt]\n";
    code << "    xor rax, rax\n";
    code << "    call printf\n";
    code << "    mov rsp, rbp\n";
    code << "    pop rbp\n";
    code << "    ret\n\n";

    code << ".data\n";
    code << "int_fmt: .asciz \"%ld\\n\"\n";
    code << dataSection.str();
}

std::string CodeGenerator::getCode() const {
    return code.str();
}

void CodeGenerator::enableOptimizations(bool constantFold, bool deadCode) {
    enableConstantFolding = constantFold;
    enableDeadCodeElimination = deadCode;
}
