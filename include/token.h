#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <map>

enum class TokenType {
    // Keywords (C language)
    IF, ELSE, WHILE, FOR, RETURN,
    INT, LONG, FLOAT, DOUBLE, CHAR, SHORT, UNSIGNED, VOID,
    STRUCT, TYPEDEF, CONST, STATIC,
    BREAK, CONTINUE, SWITCH, CASE, DEFAULT, DO,

    // Literals
    NUM, FLOAT_LIT, ID, STRING_LIT,

    // Operators
    PLUS, MINUS, MUL, DIV, MOD, ASSIGN,
    EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT,
    QUESTION, // ? for ternary
    INCREMENT, DECREMENT, // ++ and --

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, ARROW, DOT,

    // Special
    END, ERR
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    // For numeric literals
    union {
        long long intValue;
        unsigned long long uintValue;
        double floatValue;
    };

    Token(TokenType t = TokenType::ERR, const std::string& lex = "", int l = 0, int c = 0)
        : type(t), lexeme(lex), line(l), column(c), intValue(0) {}

    std::string toString() const;
    static std::string typeToString(TokenType type);
};

// Keywords map
extern std::map<std::string, TokenType> keywords;

#endif // TOKEN_H
