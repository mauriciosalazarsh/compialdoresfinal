# Compilador de C a x86-64

## Proyecto Final - CS3402 Compiladores, UTEC

---

# Integrantes

- Integrante 1
- Integrante 2
- Integrante 3

---

# Objetivo del Proyecto

Desarrollar un compilador que traduce un subconjunto del lenguaje C a codigo ensamblador x86-64

## Caracteristicas principales:
- Implementado en C++
- Arquitectura modular de 4 fases
- Genera codigo ejecutable para Linux x86-64
- Incluye optimizaciones

---

# Arquitectura del Compilador

```
Codigo Fuente (.c) -> Scanner -> Parser -> Semantic -> CodeGen -> Codigo x86-64 (.s)
```

## 4 Fases:
1. **Scanner**: Analisis lexico (tokenizacion)
2. **Parser**: Analisis sintactico (construccion del AST)
3. **Semantic**: Analisis semantico (verificacion de tipos)
4. **CodeGen**: Generacion de codigo x86-64

---

# Estructura del Proyecto

```
Final/
|-- src/
|   |-- main.cpp          # Punto de entrada
|   |-- scanner.cpp       # Analizador lexico
|   |-- parser.cpp        # Analizador sintactico
|   |-- semantic.cpp      # Analizador semantico
|   |-- codegen.cpp       # Generador de codigo
|   |-- ast.cpp           # Nodos del AST
|   |-- symboltable.cpp   # Tabla de simbolos
|-- include/
|   |-- scanner.h, parser.h, ast.h, visitor.h
|   |-- semantic.h, codegen.h, symboltable.h
|-- tests/                # 18 casos de prueba
|-- visualizer/           # Bonus: visualizador web
```

---

# Fase 1: Scanner (Analisis Lexico)

Convierte el codigo fuente en una secuencia de tokens.

## Clase Scanner

```cpp
class Scanner {
private:
    std::string input;
    size_t pos;
    int line, column;
    char current;

    void advance();
    char peek(int offset = 1);
    void skipWhitespace();
    void skipComment();
    Token scanNumber();
    Token scanIdentifier();
    Token scanString();

public:
    Scanner(const std::string& src);
    Token nextToken();
    std::vector<Token> tokenize();
};
```

---

# Tokens Reconocidos

| Categoria | Tokens | Ejemplos |
|-----------|--------|----------|
| Palabras clave | int, long, float, unsigned, void | `int x;` |
| Control | if, else, while, for, return | `if (x > 0)` |
| Extensiones | typedef | `typedef int entero;` |
| Literales | NUM, FLOAT_LIT, STRING_LIT | `42`, `3.14`, `"hola"` |
| Identificadores | ID | `variable`, `funcion` |
| Operadores | +, -, *, /, %, ==, !=, <, >, <=, >= | `a + b` |
| Delimitadores | (, ), {, }, [, ], ;, , | `func(a, b)` |

---

# Escaneo de Numeros

```cpp
Token Scanner::scanNumber() {
    std::string num;
    bool isFloat = false;

    while (std::isdigit(current)) {
        num += current;
        advance();
    }

    // detectar punto decimal
    if (current == '.' && std::isdigit(peek())) {
        isFloat = true;
        num += current;
        advance();
        while (std::isdigit(current)) {
            num += current;
            advance();
        }
    }

    // sufijo de tipo (u, U, L, l, f, F)
    if (current == 'u' || current == 'U') { advance(); }
    if (current == 'L' || current == 'l') { advance(); }
    if (current == 'f' || current == 'F') { isFloat = true; advance(); }

    return Token(isFloat ? TokenType::FLOAT_LIT : TokenType::NUM, num);
}
```

---

# Fase 2: Parser (Analisis Sintactico)

Construye un Arbol de Sintaxis Abstracta (AST) usando parsing recursivo descendente.

## Clase Parser

```cpp
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::map<std::string, DataType> typeAliases;

    Token peek(int offset = 0);
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type);
    void expect(TokenType type, const std::string& msg);

    DataType parseType();
    void parseTypedef();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseTernary();
    std::unique_ptr<Expr> parseLogicalOr();

public:
    std::unique_ptr<Program> parse();
};
```

---

# Jerarquia del AST

```
                    ASTNode
                   /       \
                Expr       Stmt
               / | \       / | \
     BinaryExpr  |  TernaryExpr  |  WhileStmt
         UnaryExpr       VarDeclStmt  IfStmt
         LiteralExpr     ForStmt      BlockStmt
         CallExpr        ReturnStmt
```

## Patron Visitor
- Permite multiples recorridos del arbol
- SemanticAnalyzer y CodeGenerator implementan Visitor

---

# Precedencia de Operadores

| Nivel | Operadores | Funcion |
|-------|------------|---------|
| 1 (menor) | ?: (ternario) | `parseTernary()` |
| 2 | \|\| | `parseLogicalOr()` |
| 3 | && | `parseLogicalAnd()` |
| 4 | ==, != | `parseEquality()` |
| 5 | <, >, <=, >= | `parseRelational()` |
| 6 | +, - | `parseAdditive()` |
| 7 | *, /, % | `parseMultiplicative()` |
| 8 (mayor) | -, ! (unarios) | `parseUnary()` |

---

# Soporte para Typedef

```cpp
void Parser::parseTypedef() {
    expect(TokenType::TYPEDEF, "Expected 'typedef'");
    DataType baseType = parseType();

    std::string aliasName = peek().lexeme;
    expect(TokenType::ID, "Expected type alias name");
    expect(TokenType::SEMICOLON, "Expected ';'");

    typeAliases[aliasName] = baseType;
}

bool Parser::isTypeToken() {
    if (check(TokenType::INT) || check(TokenType::LONG) ||
        check(TokenType::FLOAT) || check(TokenType::UNSIGNED)) {
        return true;
    }
    // verificar aliases de typedef
    if (check(TokenType::ID)) {
        return typeAliases.find(peek().lexeme) != typeAliases.end();
    }
    return false;
}
```

---

# Fase 3: Analisis Semantico

Verifica la correccion del programa usando el patron Visitor.

## Clase SemanticAnalyzer

```cpp
class SemanticAnalyzer : public Visitor {
private:
    SymbolTable& symbolTable;
    DataType currentFunctionReturnType;
    bool hasErrors;
    std::string errors;

    bool areTypesCompatible(DataType expected, DataType actual);
    DataType getCommonType(DataType t1, DataType t2);
    void error(const std::string& message);

public:
    void visit(BinaryExpr* node) override;
    void visit(TernaryExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(FunctionDecl* node) override;
};
```

---

# Sistema de Tipos

## Tipos soportados:
- `int` - entero 32 bits con signo
- `long` - entero 64 bits con signo
- `unsigned int` - entero 32 bits sin signo
- `float` - punto flotante 64 bits

## Promocion automatica de tipos:

```cpp
DataType SemanticAnalyzer::getCommonType(DataType t1, DataType t2) {
    if (t1 == t2) return t1;

    // float domina sobre todos
    if (t1 == DataType::FLOAT || t2 == DataType::FLOAT)
        return DataType::FLOAT;

    // long domina sobre int
    if (t1 == DataType::LONG || t2 == DataType::LONG)
        return DataType::LONG;

    // uint con int -> long
    if ((t1 == DataType::UINT && t2 == DataType::INT) ||
        (t1 == DataType::INT && t2 == DataType::UINT))
        return DataType::LONG;

    return t1;
}
```

---

# Tabla de Simbolos

```cpp
struct Symbol {
    std::string name;
    DataType type;
    bool isMutable;
    bool isParameter;
    int offset;  // offset en el stack frame
    std::vector<int> arrayDimensions;
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol>> scopes;
    std::map<std::string, FunctionSymbol> functions;
    int currentOffset;

public:
    void enterScope();
    void exitScope();
    bool declareVariable(const std::string& name, const Symbol& sym);
    Symbol* lookup(const std::string& name);
    int allocateStackSpace(int size);
};
```

---

# Fase 4: Generacion de Codigo

Produce ensamblador x86-64 en sintaxis Intel, siguiendo System V ABI.

## Clase CodeGenerator

```cpp
class CodeGenerator : public Visitor {
private:
    SymbolTable& symbolTable;
    std::stringstream code;
    std::stringstream dataSection;
    int labelCounter;
    bool enableConstantFolding;
    bool enableDeadCodeElimination;

    std::string newLabel();
    void emit(const std::string& instruction);
    void emitLabel(const std::string& label);
    void generatePrologue(const std::string& func, int stackSize);
    void generateEpilogue();

public:
    void visit(BinaryExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(IfStmt* node) override;
    std::string getCode() const;
};
```

---

# Convencion de Llamadas (System V ABI)

| Registro | Uso |
|----------|-----|
| rax | Valor de retorno, acumulador |
| rbx | Registro temporal (callee-saved) |
| rdi, rsi, rdx, rcx, r8, r9 | Argumentos 1-6 |
| rbp | Base pointer (marco de pila) |
| rsp | Stack pointer |
| xmm0-xmm1 | Operaciones de punto flotante |

---

# Estructura del Stack Frame

```
Direcciones altas
+---------------------------+
| Argumentos (arg3,arg2,arg1)| [rbp+24]
+---------------------------+
| Direccion de retorno      | [rbp+8]
+---------------------------+
| RBP anterior (saved rbp)  | [rbp]     <- rbp
+---------------------------+
| Variables locales         | [rbp-8]
+---------------------------+
| Espacio temporal          | [rbp-16]  <- rsp
+---------------------------+
Direcciones bajas
```

---

# Generacion de Prologo y Epilogo

```cpp
void CodeGenerator::generatePrologue(const std::string& func, int stackSize) {
    emitLabel(func);
    emit("push rbp");
    emit("mov rbp, rsp");

    // alinear a 16 bytes (requerido por ABI)
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
```

---

# Generacion de Expresiones Binarias

```cpp
void CodeGenerator::visit(BinaryExpr* node) {
    node->left->accept(this);
    emit("push rax");

    node->right->accept(this);
    emit("mov rbx, rax");
    emit("pop rax");

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
}
```

---

# Generacion de Expresiones Ternarias

```cpp
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
```

---

# Generacion de If-Else

```cpp
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
```

---

# Generacion de While

```cpp
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
```

---

# Optimizacion 1: Constant Folding

Evalua expresiones constantes en tiempo de compilacion.

```cpp
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

            emit("mov rax, " + std::to_string(result));
            return;
        }
    }
}
```

**Ejemplo:** `int x = 5 + 10 * 2;` se evalua como `mov rax, 25`

---

# Optimizacion 2: Dead Code Elimination

Omite codigo que nunca se ejecutara.

```cpp
void CodeGenerator::visit(IfStmt* node) {
    if (auto lit = dynamic_cast<LiteralExpr*>(node->condition.get())) {
        if (lit->value == "0") {
            // condicion siempre falsa: solo generar else
            if (node->elseBranch) {
                node->elseBranch->accept(this);
            }
            return;
        } else {
            // condicion siempre verdadera: solo generar then
            node->thenBranch->accept(this);
            return;
        }
    }
}
```

**Ejemplo:** `if (0) { z = 100; }` no genera codigo

---

# Extension 1: Expresiones Ternarias

```c
int max = (x > y) ? x : y;
int result = (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
```

## Implementacion en el Parser:

```cpp
std::unique_ptr<Expr> Parser::parseTernary() {
    auto expr = parseLogicalOr();

    if (match(TokenType::QUESTION)) {
        auto trueExpr = parseExpression();
        expect(TokenType::COLON, "Expected ':'");
        auto falseExpr = parseExpression();
        return std::make_unique<TernaryExpr>(
            std::move(expr),
            std::move(trueExpr),
            std::move(falseExpr)
        );
    }

    return expr;
}
```

---

# Extension 2: Typedef

```c
typedef int entero;
typedef long numero_grande;
typedef float decimal;

entero x = 10;
numero_grande big = 1000000;
decimal pi = 3.14;
```

El parser mantiene un mapa `typeAliases` que asocia nombres de alias con sus tipos base.

---

# Implementacion Adicional: Tipos Numericos

## Tipos implementados:

| Tipo | Descripcion | Registros |
|------|-------------|-----------|
| int | entero 32 bits con signo | eax, rax |
| long | entero 64 bits con signo | rax |
| unsigned int | entero 32 bits sin signo | eax, rax |
| float | punto flotante 64 bits | xmm0, xmm1 |

## Conversion de tipos:

```cpp
void CodeGenerator::convertType(DataType from, DataType to) {
    if (from == to) return;

    // int/long/uint -> float
    if ((from == DataType::INT || from == DataType::LONG) && to == DataType::FLOAT) {
        emit("cvtsi2sd xmm0, rax");
    }
    // float -> int/long
    else if (from == DataType::FLOAT && to == DataType::INT) {
        emit("cvttsd2si rax, xmm0");
    }
    // int -> long
    else if (from == DataType::INT && to == DataType::LONG) {
        emit("cdqe");
    }
}
```

---

# Casos de Prueba: Funciones (3)

| Test | Descripcion | Salida |
|------|-------------|--------|
| test_func1.c | Funcion suma simple | 15 |
| test_func2.c | Funcion cuadrado anidada | 49 |
| test_func3.c | Factorial recursivo | 120 |

## test_func3.c - Factorial recursivo:

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    printf("%d\n", result);  // Output: 120
    return 0;
}
```

---

# Casos de Prueba: Base (5)

| Test | Descripcion | Salida |
|------|-------------|--------|
| test_base1.c | Aritmetica basica | 30, 10, 200, 2 |
| test_base2.c | Sentencias if-else | 10, 1 |
| test_base3.c | Bucle while | 45 |
| test_base4.c | Bucle for | 45 |
| test_base5.c | Expresiones complejas | 11, 16, 4 |

---

# Casos de Prueba: Extensiones (5)

| Test | Descripcion | Salida |
|------|-------------|--------|
| test_ext1.c | Expresiones ternarias | 10, 5 |
| test_ext2.c | Unsigned int | 150 |
| test_ext3.c | Long int | 3000000 |
| test_ext4.c | Float | 6.280000 |
| test_ext5.c | Typedef | 30, 1000000, 3.140000 |

---

# Casos de Prueba: Optimizacion (5)

| Test | Descripcion | Salida |
|------|-------------|--------|
| test_opt1.c | Constant folding | 5, 50, 25 |
| test_opt2.c | Dead code elimination | 30 |
| test_opt3.c | Strength reduction | 16, 32 |
| test_opt4.c | Common subexpression | 16 |
| test_opt5.c | Loop optimization | 100 |

---

# Ejemplo: Codigo Fuente

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    printf("%d\n", result);
    return 0;
}
```

---

# Ejemplo: Codigo x86-64 Generado

```asm
.intel_syntax noprefix
.text
.global main

factorial:
    push rbp
    mov rbp, rsp
    mov rax, [rbp + 16]      ; cargar parametro n
    push rax
    mov rax, 1
    mov rbx, rax
    pop rax
    cmp rax, rbx
    setle al
    movzx rax, al
    test rax, rax
    jz .L1                    ; si n > 1, saltar
    mov rax, 1                ; return 1
    mov rsp, rbp
    pop rbp
    ret
.L1:
    mov rax, [rbp + 16]       ; cargar n
    push rax
    sub rsp, 8                ; alinear stack
    mov rax, [rbp + 16]
    push rax
    mov rax, 1
    mov rbx, rax
    pop rax
    sub rax, rbx              ; n - 1
    push rax
    call factorial            ; llamada recursiva
    add rsp, 16
    mov rbx, rax
    pop rax
    imul rax, rbx             ; n * factorial(n-1)
    mov rsp, rbp
    pop rbp
    ret
```

---

# Ejemplo: main() Generado

```asm
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    sub rsp, 8
    mov rax, 5
    push rax
    call factorial
    add rsp, 16
    mov [rbp - 8], rax        ; result = factorial(5)
    mov rax, [rbp - 8]
    push rax
    lea rax, [.STR0]
    push rax
    pop rdi
    pop rsi
    xor eax, eax
    call printf
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

.data
int_fmt: .asciz "%ld\n"
.STR0: .asciz "%d\n"
```

---

# Instrucciones de Uso

## Compilar el compilador:
```bash
make
make clean  # limpiar
```

## Linux x86-64:
```bash
./compiler archivo.c
gcc -no-pie output.s -o program
./program
```

## macOS (Apple Silicon):
```bash
./run_x86.sh archivo.c
```

## Windows (WSL2):
```bash
./compiler archivo.c
gcc -no-pie output.s -o program
./program
```

## Windows (Docker):
```powershell
docker run --rm -v ${PWD}:/work -w /work gcc:latest bash -c "./compiler tests/test_base1.c && gcc -no-pie output.s -o program && ./program"
```

---

# Visualizador (Bonus)

Herramienta web para visualizar el estado de registros y pila paso a paso.

```bash
cd visualizer
pip3 install -r requirements.txt
python3 server.py
# Abrir http://localhost:8080
```

---

# Conclusiones

## Logros:
- Implementacion completa de las 4 fases del compilador
- Soporte para multiples tipos de datos con promocion automatica
- Manejo correcto de funciones recursivas y estructuras de control
- Optimizaciones efectivas (constant folding, dead code elimination)
- Extensiones utiles (expresiones ternarias, typedef)
- 18 casos de prueba exitosos

## El proyecto demuestra:
- Analisis lexico con automatas
- Parsing recursivo descendente
- Verificacion de tipos
- Generacion de codigo x86-64
- Optimizaciones en tiempo de compilacion

---

# Trabajo Futuro

- Soporte para estructuras (struct) y uniones
- Implementacion de punteros y memoria dinamica
- Optimizaciones adicionales (register allocation, peephole)
- Soporte para mas operadores de C
- Generacion de codigo para otras arquitecturas (ARM)

---

# Referencias

1. Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2006). *Compilers: Principles, Techniques, and Tools* (2nd ed.). Addison-Wesley.

2. Intel Corporation. (2021). *Intel 64 and IA-32 Architectures Software Developer's Manual*.

3. System V Application Binary Interface: AMD64 Architecture Processor Supplement.

4. Muchnick, S. S. (1997). *Advanced Compiler Design and Implementation*. Morgan Kaufmann.

---

# Gracias

## Repositorio:
github.com/mauriciosalazarsh/compialdoresfinal

## Preguntas?
