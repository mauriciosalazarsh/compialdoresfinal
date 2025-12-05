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

std::string CodeGenerator::reg(const std::string& r) {
    return "%" + r;
}

std::string CodeGenerator::imm(const std::string& val) {
    return "$" + val;
}

std::string CodeGenerator::imm(int val) {
    return "$" + std::to_string(val);
}

std::string CodeGenerator::mem(const std::string& base, int offset) {
    if (offset == 0) return "(" + reg(base) + ")";
    return std::to_string(offset) + "(" + reg(base) + ")";
}

void CodeGenerator::emitLabel(const std::string& label) {
    code << label << ":\n";
}

void CodeGenerator::generatePrologue(const std::string& funcName, int stackSize) {
    emitLabel(funcName);
    emit("pushq %rbp");
    emit("movq %rsp, %rbp");
    // alinear a 16 bytes
    int alignedSize = ((stackSize + 15) & ~15);
    if (alignedSize > 0) {
        emit("subq " + imm(alignedSize) + ", %rsp");
    }
}

void CodeGenerator::generateEpilogue() {
    emit("leave");
    emit("ret");
}

void CodeGenerator::loadVariable(const std::string& varName, DataType type) {
    Symbol* sym = symbolTable.lookup(varName);
    if (!sym) return;

    if (sym->isParameter) {
        emit("movq " + std::to_string(sym->offset) + "(%rbp), %rax");
    } else {
        emit("movq " + std::to_string(sym->offset) + "(%rbp), %rax");
    }

    if (type == DataType::FLOAT) {
        emit("movq %rax, %xmm0");
    }
}

void CodeGenerator::storeVariable(const std::string& varName, DataType type) {
    Symbol* sym = symbolTable.lookup(varName);
    if (!sym) return;

    if (type == DataType::FLOAT) {
        emit("movq %xmm0, %rax");
    }

    if (sym->isParameter) {
        emit("movq %rax, " + std::to_string(sym->offset) + "(%rbp)");
    } else {
        emit("movq %rax, " + std::to_string(sym->offset) + "(%rbp)");
    }
}

void CodeGenerator::convertType(DataType from, DataType to) {
    if (from == to) return;

    // int/long/uint -> float
    if ((from == DataType::INT || from == DataType::LONG || from == DataType::UINT) &&
        to == DataType::FLOAT) {
        emit("cvtsi2sdq %rax, %xmm0");
    }
    // float -> int/long
    else if (from == DataType::FLOAT &&
             (to == DataType::INT || to == DataType::LONG || to == DataType::UINT)) {
        emit("cvttsd2siq %xmm0, %rax");
    }
    // int -> long
    else if (from == DataType::INT && to == DataType::LONG) {
        emit("cltq");
    }
    // uint -> long
    else if (from == DataType::UINT && to == DataType::LONG) {
        emit("movl %eax, %eax");
    }
}

void CodeGenerator::computeArrayOffset(const std::vector<std::unique_ptr<Expr>>& indices,
                                      const std::vector<int>& dimensions) {
    if (indices.empty()) return;

    indices[0]->accept(this);
    emit("pushq %rax");

    for (size_t i = 1; i < indices.size(); ++i) {
        int dimProduct = 1;
        for (size_t j = i; j < dimensions.size(); ++j) {
            dimProduct *= dimensions[j];
        }

        emit("popq %rax");
        emit("imulq " + imm(dimProduct) + ", %rax");
        emit("pushq %rax");

        indices[i]->accept(this);
        emit("popq %rbx");
        emit("addq %rbx, %rax");
        emit("pushq %rax");
    }

    emit("popq %rax");
    emit("imulq " + imm(8) + ", %rax");
}

void CodeGenerator::visit(BinaryExpr* node) {
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

            emit("movq " + imm(std::to_string(result)) + ", %rax");
            return;
        }
    }

no_fold:
    node->left->accept(this);
    emit("pushq %rax");

    node->right->accept(this);
    emit("movq %rax, %rbx");
    emit("popq %rax");

    if (node->exprType == DataType::FLOAT) {
        emit("movq %rax, %xmm0");
        emit("movq %rbx, %xmm1");

        if (node->op == "+") emit("addsd %xmm1, %xmm0");
        else if (node->op == "-") emit("subsd %xmm1, %xmm0");
        else if (node->op == "*") emit("mulsd %xmm1, %xmm0");
        else if (node->op == "/") emit("divsd %xmm1, %xmm0");

        emit("movq %xmm0, %rax");
    } else {

        if (node->op == "+") {
            emit("addq %rbx, %rax");
        } else if (node->op == "-") {
            emit("subq %rbx, %rax");
        } else if (node->op == "*") {
            emit("imulq %rbx, %rax");
        } else if (node->op == "/") {
            emit("cqto");
            emit("idivq %rbx");
        } else if (node->op == "%") {
            emit("cqto");
            emit("idivq %rbx");
            emit("movq %rdx, %rax");
        }

        bool isUnsigned = (node->left->exprType == DataType::UINT);

        if (node->op == "<") {
            emit("cmpq %rbx, %rax");
            emit(isUnsigned ? "setb %al" : "setl %al");
            emit("movzbq %al, %rax");
        } else if (node->op == "<=") {
            emit("cmpq %rbx, %rax");
            emit(isUnsigned ? "setbe %al" : "setle %al");
            emit("movzbq %al, %rax");
        } else if (node->op == ">") {
            emit("cmpq %rbx, %rax");
            emit(isUnsigned ? "seta %al" : "setg %al");
            emit("movzbq %al, %rax");
        } else if (node->op == ">=") {
            emit("cmpq %rbx, %rax");
            emit(isUnsigned ? "setae %al" : "setge %al");
            emit("movzbq %al, %rax");
        } else if (node->op == "==") {
            emit("cmpq %rbx, %rax");
            emit("sete %al");
            emit("movzbq %al, %rax");
        } else if (node->op == "!=") {
            emit("cmpq %rbx, %rax");
            emit("setne %al");
            emit("movzbq %al, %rax");
        }
        // logicas
        else if (node->op == "&&") {
            emit("andq %rbx, %rax");
        } else if (node->op == "||") {
            emit("orq %rbx, %rax");
        }
    }
}

void CodeGenerator::visit(UnaryExpr* node) {
    node->operand->accept(this);

    if (node->op == "-") {
        if (node->exprType == DataType::FLOAT) {
            emit("movq %rax, %xmm0");
            emit("xorpd %xmm1, %xmm1");
            emit("subsd %xmm0, %xmm1");
            emit("movq %xmm1, %rax");
        } else {
            emit("negq %rax");
        }
    } else if (node->op == "!") {
        emit("testq %rax, %rax");
        emit("setz %al");
        emit("movzbq %al, %rax");
    }
}

void CodeGenerator::visit(TernaryExpr* node) {
    std::string falseLabel = newLabel();
    std::string endLabel = newLabel();

    node->condition->accept(this);
    emit("testq %rax, %rax");
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
        emit("movsd " + label + "(%rip), %xmm0");
        emit("movq %xmm0, %rax");
    } else if (node->exprType == DataType::STRING) {
        std::string label = newStringLabel();
        std::string escaped = escapeString(node->value);
        dataSection << label << ": .asciz \"" << escaped << "\"\n";
        emit("leaq " + label + "(%rip), %rax");
    } else {
        emit("movq " + imm(node->value) + ", %rax");
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
        emit("movq " + std::to_string(sym->offset) + "(%rbp), %rbx");
    } else {
        emit("leaq " + std::to_string(sym->offset) + "(%rbp), %rbx");
    }

    emit("addq %rax, %rbx");
    emit("movq (%rbx), %rax");
}

void CodeGenerator::visit(CallExpr* node) {
    // println
    if (node->name == "println") {
        if (!node->args.empty()) {
            node->args[0]->accept(this);
            emit("movq %rax, %rsi");
            emit("leaq int_fmt(%rip), %rdi");
            emit("movl $0, %eax");
            emit("subq $8, %rsp");
            emit("call printf@PLT");
            emit("addq $8, %rsp");
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
            emit("movq %rax, %rdi");

            node->args[1]->accept(this);
            emit("movq %rax, %xmm0");

            emit("movl $1, %eax");
            emit("call printf@PLT");
            return;
        }

        for (int i = numArgs - 1; i >= 0; --i) {
            node->args[i]->accept(this);
            if (i < (int)argRegs.size()) {
                emit("movq %rax, %" + argRegs[i]);
            } else {
                emit("pushq %rax"); // Fallback for >6 args (unlikely in examples but safe)
            }
        }

        emit("movl $0, %eax");
        emit("call printf@PLT");
        return;
    }

    std::vector<std::string> argRegsUser = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    int numArgs = node->args.size();

    for (int i = numArgs - 1; i >= 0; --i) {
        node->args[i]->accept(this);
        if (i < (int)argRegsUser.size()) {
            emit("movq %rax, %" + argRegsUser[i]);
        } else {
            emit("pushq %rax");
        }
    }

    // alinear stack a 16 bytes antes de call
    bool needAlignment = (numArgs <= 6) && (numArgs % 2 == 1);
    if (needAlignment) {
        emit("subq $8, %rsp");
    }

    emit("call " + node->name);

    if (needAlignment) {
        emit("addq $8, %rsp");
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
    emit("pushq %rax");

    if (auto arrayAccess = dynamic_cast<ArrayAccessExpr*>(node->target.get())) {
        auto idExpr = dynamic_cast<IdentifierExpr*>(arrayAccess->array.get());
        if (idExpr) {
            Symbol* sym = symbolTable.lookup(idExpr->name);
            if (sym) {
                computeArrayOffset(arrayAccess->indices, sym->arrayDimensions);

                if (sym->isParameter) {
                    emit("movq " + std::to_string(sym->offset) + "(%rbp), %rbx");
                } else {
                    emit("leaq " + std::to_string(sym->offset) + "(%rbp), %rbx");
                }

                emit("addq %rax, %rbx");
                emit("popq %rax");
                emit("movq %rax, (%rbx)");
            }
        }
    } else if (auto id = dynamic_cast<IdentifierExpr*>(node->target.get())) {
        emit("popq %rax");
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
    if (enableDeadCodeElimination) {
        if (auto literal = dynamic_cast<LiteralExpr*>(node->condition.get())) {
            long long val = 0;
            try {
                val = std::stoll(literal->value);
            } catch (...) {
                val = 1; 
            }

            if (val == 0) {
                if (node->elseBranch) {
                    node->elseBranch->accept(this);
                }
                return;
            } else {
                node->thenBranch->accept(this);
                return;
            }
        }
    }

    std::string elseLabel = newLabel();
    std::string endLabel = newLabel();

    node->condition->accept(this);
    emit("testq %rax, %rax");

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
    emit("testq %rax, %rax");
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
    emit("pushq %rax");
    node->end->accept(this);
    emit("popq %rbx");
    emit("cmpq %rax, %rbx");
    emit("jge " + endLabel);

    node->body->accept(this);

    loadVariable(node->varName, DataType::INT);
    emit("incq %rax");
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

    // Registros para parámetros según ABI x86-64
    std::vector<std::string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

    // Reservar espacio en stack para los parámetros (se guardarán como variables locales)
    for (size_t i = 0; i < node->params.size() && i < paramRegs.size(); ++i) {
        Symbol paramSym;
        paramSym.name = node->params[i].name;
        paramSym.type = node->params[i].type;
        paramSym.isParameter = false;  // Ahora se tratan como variables locales
        paramSym.offset = symbolTable.allocateStackSpace(8);
        paramSym.arrayDimensions = node->params[i].arrayDimensions;
        symbolTable.declareVariable(node->params[i].name, paramSym);
    }

    preDeclareVariables(node->body.get());
    int stackSize = -symbolTable.getCurrentOffset();

    generatePrologue(node->name, stackSize);

    // Guardar los parámetros de los registros al stack
    for (size_t i = 0; i < node->params.size() && i < paramRegs.size(); ++i) {
        Symbol* sym = symbolTable.lookup(node->params[i].name);
        if (sym) {
            emit("movq %" + paramRegs[i] + ", " + std::to_string(sym->offset) + "(%rbp)");
        }
    }

    node->body->accept(this);

    if (node->returnType == DataType::VOID) {
        generateEpilogue();
    }

    symbolTable.exitScope();
}

void CodeGenerator::visit(Program* node) {
    code << ".text\n";
    code << ".global main\n\n";

    for (auto& func : node->functions) {
        func->accept(this);
        code << "\n";
    }

    code << "print_int:\n";
    code << "    pushq %rbp\n";
    code << "    movq %rsp, %rbp\n";
    code << "    movq %rdi, %rsi\n";
    code << "    leaq int_fmt(%rip), %rdi\n";
    code << "    movl $0, %eax\n";
    code << "    call printf@PLT\n";
    code << "    leave\n";
    code << "    ret\n\n";

    code << ".data\n";
    code << "int_fmt: .asciz \"%ld\\n\"\n";
    code << dataSection.str();
    code << ".section .note.GNU-stack,\"\",@progbits\n";
}

std::string CodeGenerator::getCode() const {
    return code.str();
}

void CodeGenerator::enableOptimizations(bool constantFold, bool deadCode) {
    enableConstantFolding = constantFold;
    enableDeadCodeElimination = deadCode;
}
