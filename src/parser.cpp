#include "../include/parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& toks) : tokens(toks), current(0) {}

Token Parser::peek(int offset) {
    size_t pos = current + offset;
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    return tokens.back();
}

Token Parser::advance() {
    if (current < tokens.size() - 1) {
        return tokens[current++];
    }
    return tokens.back();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    return peek().type == type;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!match(type)) {
        throw std::runtime_error("Parse error at line " + std::to_string(peek().line) +
                               ": " + message + " (got " + peek().lexeme + ")");
    }
}

DataType Parser::parseType() {
    // tipos unsigned
    if (match(TokenType::UNSIGNED)) {
        if (match(TokenType::INT)) return DataType::UINT;
        if (match(TokenType::LONG)) return DataType::UINT;
        return DataType::UINT;
    }
    if (match(TokenType::INT)) return DataType::INT;
    if (match(TokenType::LONG)) return DataType::LONG;
    if (match(TokenType::FLOAT)) return DataType::FLOAT;
    if (match(TokenType::VOID)) return DataType::VOID;

    // verificar aliases de typedef
    if (check(TokenType::ID)) {
        std::string typeName = peek().lexeme;
        auto it = typeAliases.find(typeName);
        if (it != typeAliases.end()) {
            advance(); // consume the alias name
            return it->second;
        }
    }

    throw std::runtime_error("Expected type at line " + std::to_string(peek().line));
}

void Parser::parseTypedef() {
    expect(TokenType::TYPEDEF, "Expected 'typedef'");

    DataType baseType = parseType();

    std::string aliasName = peek().lexeme;
    expect(TokenType::ID, "Expected type alias name");

    expect(TokenType::SEMICOLON, "Expected ';' after typedef");
    typeAliases[aliasName] = baseType;
}

bool Parser::isTypeToken() {
    if (check(TokenType::INT) || check(TokenType::LONG) ||
        check(TokenType::FLOAT) || check(TokenType::UNSIGNED) ||
        check(TokenType::VOID)) {
        return true;
    }
    if (check(TokenType::ID)) {
        return typeAliases.find(peek().lexeme) != typeAliases.end();
    }
    return false;
}

std::unique_ptr<Program> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<Program> Parser::parseProgram() {
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    while (!check(TokenType::END)) {
        if (check(TokenType::TYPEDEF)) {
            parseTypedef();
            continue;
        }
        functions.push_back(parseFunctionDecl());
    }

    return std::make_unique<Program>(std::move(functions));
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    DataType returnType = parseType();

    std::string name = peek().lexeme;
    expect(TokenType::ID, "Expected function name");

    expect(TokenType::LPAREN, "Expected '('");

    std::vector<Parameter> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Parameter param;
            param.type = parseType();
            param.name = peek().lexeme;
            expect(TokenType::ID, "Expected parameter name");

            while (match(TokenType::LBRACKET)) {
                if (match(TokenType::NUM)) {
                    param.arrayDimensions.push_back(std::stoi(tokens[current-1].lexeme));
                } else {
                    param.arrayDimensions.push_back(-1);
                }
                expect(TokenType::RBRACKET, "Expected ']'");
            }

            params.push_back(param);
        } while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN, "Expected ')'");

    auto body = parseBlock();

    return std::make_unique<FunctionDecl>(name, std::move(params), returnType, std::move(body));
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (isTypeToken()) {
        return parseVarDecl();
    }
    if (check(TokenType::IF)) {
        return parseIfStmt();
    }
    if (check(TokenType::WHILE)) {
        return parseWhileStmt();
    }
    if (check(TokenType::FOR)) {
        return parseForStmt();
    }
    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }
    if (check(TokenType::RETURN)) {
        return parseReturnStmt();
    }

    auto expr = parseExpression();

    if (match(TokenType::ASSIGN)) {
        auto value = parseExpression();
        match(TokenType::SEMICOLON);
        return std::make_unique<AssignStmt>(std::move(expr), std::move(value));
    }

    match(TokenType::SEMICOLON);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
    DataType type = parseType();

    std::string name = peek().lexeme;
    expect(TokenType::ID, "Expected variable name");

    std::vector<int> arrayDimensions;
    while (match(TokenType::LBRACKET)) {
        if (match(TokenType::NUM)) {
            arrayDimensions.push_back(std::stoi(tokens[current-1].lexeme));
        } else {
            arrayDimensions.push_back(-1);
        }
        expect(TokenType::RBRACKET, "Expected ']'");
    }

    std::unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }

    match(TokenType::SEMICOLON);
    auto decl = std::make_unique<VarDeclStmt>(true, name, type, std::move(initializer));
    decl->arrayDimensions = arrayDimensions;

    return decl;
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    expect(TokenType::IF, "Expected 'if'");
    expect(TokenType::LPAREN, "Expected '('");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')'");

    auto thenBranch = parseStatement();

    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    expect(TokenType::WHILE, "Expected 'while'");
    expect(TokenType::LPAREN, "Expected '('");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')'");

    auto body = parseStatement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    expect(TokenType::FOR, "Expected 'for'");
    expect(TokenType::LPAREN, "Expected '('");

    DataType initType = parseType();
    std::string varName = peek().lexeme;
    expect(TokenType::ID, "Expected loop variable");
    expect(TokenType::ASSIGN, "Expected '='");
    auto start = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';'");

    auto condition = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';'");

    // saltar incremento
    while (!check(TokenType::RPAREN) && !check(TokenType::END)) {
        advance();
    }

    expect(TokenType::RPAREN, "Expected ')'");

    auto body = parseStatement();

    // extraer valor final de la condicion
    BinaryExpr* binCond = dynamic_cast<BinaryExpr*>(condition.get());
    std::unique_ptr<Expr> end;
    if (binCond && (binCond->op == "<" || binCond->op == "<=")) {
        end = std::move(binCond->right);
    } else {
        end = std::make_unique<LiteralExpr>("10", DataType::INT);
    }

    return std::make_unique<ForStmt>(varName, std::move(start), std::move(end), std::move(body));
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");

    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
        statements.push_back(parseStatement());
    }

    expect(TokenType::RBRACE, "Expected '}'");

    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    expect(TokenType::RETURN, "Expected 'return'");

    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE) && !check(TokenType::END)) {
        value = parseExpression();
    }

    match(TokenType::SEMICOLON);

    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseTernary();
}

std::unique_ptr<Expr> Parser::parseTernary() {
    auto expr = parseLogicalOr();

    if (match(TokenType::QUESTION)) {
        auto trueExpr = parseExpression();
        expect(TokenType::COLON, "Expected ':' in ternary expression");
        auto falseExpr = parseExpression();
        return std::make_unique<TernaryExpr>(std::move(expr), std::move(trueExpr), std::move(falseExpr));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (match(TokenType::OR)) {
        std::string op = "||";
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseEquality();

    while (match(TokenType::AND)) {
        std::string op = "&&";
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseRelational();

    while (check(TokenType::EQ) || check(TokenType::NE)) {
        Token op = advance();
        auto right = parseRelational();
        left = std::make_unique<BinaryExpr>(std::move(left), op.lexeme, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto left = parseAdditive();

    while (check(TokenType::LT) || check(TokenType::GT) ||
           check(TokenType::LE) || check(TokenType::GE)) {
        Token op = advance();
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(std::move(left), op.lexeme, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = advance();
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(std::move(left), op.lexeme, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();

    while (check(TokenType::MUL) || check(TokenType::DIV) || check(TokenType::MOD)) {
        Token op = advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(std::move(left), op.lexeme, std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::MINUS) || check(TokenType::NOT)) {
        Token op = advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(op.lexeme, std::move(operand));
    }

    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LBRACKET)) {
            std::vector<std::unique_ptr<Expr>> indices;
            indices.push_back(parseExpression());
            expect(TokenType::RBRACKET, "Expected ']'");

            while (match(TokenType::LBRACKET)) {
                indices.push_back(parseExpression());
                expect(TokenType::RBRACKET, "Expected ']'");
            }

            expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(indices));
        } else if (match(TokenType::LPAREN)) {
            if (auto id = dynamic_cast<IdentifierExpr*>(expr.get())) {
                std::string name = id->name;
                std::vector<std::unique_ptr<Expr>> args;

                if (!check(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }

                expect(TokenType::RPAREN, "Expected ')'");
                expr = std::make_unique<CallExpr>(name, std::move(args));
            }
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::NUM)) {
        Token tok = tokens[current - 1];
        return std::make_unique<LiteralExpr>(tok.lexeme, DataType::INT);
    }

    if (match(TokenType::FLOAT_LIT)) {
        Token tok = tokens[current - 1];
        return std::make_unique<LiteralExpr>(tok.lexeme, DataType::FLOAT);
    }

    if (match(TokenType::STRING_LIT)) {
        Token tok = tokens[current - 1];
        return std::make_unique<LiteralExpr>(tok.lexeme, DataType::STRING);
    }

    if (match(TokenType::ID)) {
        Token tok = tokens[current - 1];
        return std::make_unique<IdentifierExpr>(tok.lexeme);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    throw std::runtime_error("Unexpected token at line " + std::to_string(peek().line) +
                           ": " + peek().lexeme);
}
