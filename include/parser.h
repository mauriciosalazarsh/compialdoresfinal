#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <map>

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::map<std::string, DataType> typeAliases;  

    Token peek(int offset = 0);
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type);
    void expect(TokenType type, const std::string& message);
    bool isTypeToken();
    void parseTypedef();  

    DataType parseType();
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<VarDeclStmt> parseVarDecl();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseReturnStmt();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseTernary();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();

public:
    Parser(const std::vector<Token>& toks);
    std::unique_ptr<Program> parse();
};

#endif // PARSER_H
