#include "../include/token.h"

std::map<std::string, TokenType> keywords = {
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"return", TokenType::RETURN},
    {"int", TokenType::INT},
    {"long", TokenType::LONG},
    {"float", TokenType::FLOAT},
    {"double", TokenType::DOUBLE},
    {"char", TokenType::CHAR},
    {"short", TokenType::SHORT},
    {"unsigned", TokenType::UNSIGNED},
    {"void", TokenType::VOID},
    {"struct", TokenType::STRUCT},
    {"typedef", TokenType::TYPEDEF},
    {"const", TokenType::CONST},
    {"static", TokenType::STATIC},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"switch", TokenType::SWITCH},
    {"case", TokenType::CASE},
    {"default", TokenType::DEFAULT},
    {"do", TokenType::DO}
};

std::string Token::toString() const {
    return typeToString(type) + " [" + lexeme + "] at " +
           std::to_string(line) + ":" + std::to_string(column);
}

std::string Token::typeToString(TokenType type) {
    switch(type) {
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::RETURN: return "RETURN";
        case TokenType::INT: return "INT";
        case TokenType::LONG: return "LONG";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::DOUBLE: return "DOUBLE";
        case TokenType::CHAR: return "CHAR";
        case TokenType::SHORT: return "SHORT";
        case TokenType::UNSIGNED: return "UNSIGNED";
        case TokenType::VOID: return "VOID";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::TYPEDEF: return "TYPEDEF";
        case TokenType::CONST: return "CONST";
        case TokenType::STATIC: return "STATIC";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CONTINUE: return "CONTINUE";
        case TokenType::SWITCH: return "SWITCH";
        case TokenType::CASE: return "CASE";
        case TokenType::DEFAULT: return "DEFAULT";
        case TokenType::DO: return "DO";
        case TokenType::NUM: return "NUM";
        case TokenType::FLOAT_LIT: return "FLOAT_LIT";
        case TokenType::ID: return "ID";
        case TokenType::STRING_LIT: return "STRING_LIT";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";
        case TokenType::MOD: return "MOD";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQ: return "EQ";
        case TokenType::NE: return "NE";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LE: return "LE";
        case TokenType::GE: return "GE";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::INCREMENT: return "INCREMENT";
        case TokenType::DECREMENT: return "DECREMENT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::ARROW: return "ARROW";
        case TokenType::DOT: return "DOT";
        case TokenType::END: return "END";
        case TokenType::ERR: return "ERR";
        default: return "UNKNOWN";
    }
}
