#include "../include/scanner.h"
#include <cctype>
#include <stdexcept>

Scanner::Scanner(const std::string& src) : input(src), pos(0), line(1), column(1) {
    current = input.empty() ? '\0' : input[0];
}

void Scanner::advance() {
    if (current == '\0') return;
    if (current == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    pos++;
    current = (pos < input.length()) ? input[pos] : '\0';
}

char Scanner::peek(int offset) {
    size_t peekPos = pos + offset;
    return (peekPos < input.length()) ? input[peekPos] : '\0';
}

void Scanner::skipWhitespace() {
    while (std::isspace(current)) {
        advance();
    }
}

void Scanner::skipComment() {
    if (current == '/' && peek() == '/') {
        while (current != '\n' && current != '\0') {
            advance();
        }
    } else if (current == '/' && peek() == '*') {
        advance();
        advance();
        while (true) {
            if (current == '\0') break;
            if (current == '*' && peek() == '/') {
                advance();
                advance();
                break;
            }
            advance();
        }
    }
}

void Scanner::skipPreprocessor() {
    // saltar directivas del preprocesador
    if (current == '#') {
        while (current != '\n' && current != '\0') {
            advance();
        }
    }
}

Token Scanner::scanNumber() {
    int startLine = line, startCol = column;
    std::string num;
    bool isFloat = false;
    bool isUnsigned = false;
    bool isLong = false;

    while (std::isdigit(current)) {
        num += current;
        advance();
    }

    if (current == '.' && std::isdigit(peek())) {
        isFloat = true;
        num += current;
        advance();
        while (std::isdigit(current)) {
            num += current;
            advance();
        }
    }

    // sufijo de tipo
    if (current == 'u' || current == 'U') {
        isUnsigned = true;
        advance();
    }
    if (current == 'L' || current == 'l') {
        isLong = true;
        advance();
    }
    if (current == 'f' || current == 'F') {
        isFloat = true;
        advance();
    }

    Token tok(isFloat ? TokenType::FLOAT_LIT : TokenType::NUM, num, startLine, startCol);
    if (isFloat) {
        tok.floatValue = std::stod(num);
    } else if (isUnsigned) {
        tok.uintValue = std::stoull(num);
    } else {
        tok.intValue = std::stoll(num);
    }
    return tok;
}

Token Scanner::scanIdentifier() {
    int startLine = line, startCol = column;
    std::string id;

    while (std::isalnum(current) || current == '_') {
        id += current;
        advance();
    }

    auto it = keywords.find(id);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::ID;

    return Token(type, id, startLine, startCol);
}

Token Scanner::scanString() {
    int startLine = line, startCol = column;
    std::string str;
    advance();

    while (current != '"' && current != '\0') {
        if (current == '\\') {
            advance();
            switch (current) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '\\': str += '\\'; break;
                case '"': str += '"'; break;
                default: str += current; break;
            }
        } else {
            str += current;
        }
        advance();
    }

    if (current == '"') {
        advance();
    }

    return Token(TokenType::STRING_LIT, str, startLine, startCol);
}

Token Scanner::nextToken() {
    skipWhitespace();

    // saltar directivas
    while (current == '#') {
        skipPreprocessor();
        skipWhitespace();
    }

    while (current == '/' && (peek() == '/' || peek() == '*')) {
        skipComment();
        skipWhitespace();
    }

    if (current == '\0') {
        return Token(TokenType::END, "", line, column);
    }

    int startLine = line, startCol = column;

    // numeros
    if (std::isdigit(current)) {
        return scanNumber();
    }

    // identificadores y keywords
    if (std::isalpha(current) || current == '_') {
        return scanIdentifier();
    }

    // strings
    if (current == '"') {
        return scanString();
    }

    // operadores de dos caracteres
    if (current == '=' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::EQ, "==", startLine, startCol);
    }
    if (current == '!' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::NE, "!=", startLine, startCol);
    }
    if (current == '<' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::LE, "<=", startLine, startCol);
    }
    if (current == '>' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::GE, ">=", startLine, startCol);
    }
    if (current == '&' && peek() == '&') {
        advance(); advance();
        return Token(TokenType::AND, "&&", startLine, startCol);
    }
    if (current == '|' && peek() == '|') {
        advance(); advance();
        return Token(TokenType::OR, "||", startLine, startCol);
    }
    if (current == '-' && peek() == '>') {
        advance(); advance();
        return Token(TokenType::ARROW, "->", startLine, startCol);
    }
    if (current == '+' && peek() == '+') {
        advance(); advance();
        return Token(TokenType::INCREMENT, "++", startLine, startCol);
    }
    if (current == '-' && peek() == '-') {
        advance(); advance();
        return Token(TokenType::DECREMENT, "--", startLine, startCol);
    }
    if (current == '+' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::PLUS, "+=", startLine, startCol);
    }
    if (current == '-' && peek() == '=') {
        advance(); advance();
        return Token(TokenType::MINUS, "-=", startLine, startCol);
    }

    // tokens de un caracter
    char c = current;
    advance();

    switch (c) {
        case '+': return Token(TokenType::PLUS, "+", startLine, startCol);
        case '-': return Token(TokenType::MINUS, "-", startLine, startCol);
        case '*': return Token(TokenType::MUL, "*", startLine, startCol);
        case '/': return Token(TokenType::DIV, "/", startLine, startCol);
        case '%': return Token(TokenType::MOD, "%", startLine, startCol);
        case '=': return Token(TokenType::ASSIGN, "=", startLine, startCol);
        case '<': return Token(TokenType::LT, "<", startLine, startCol);
        case '>': return Token(TokenType::GT, ">", startLine, startCol);
        case '!': return Token(TokenType::NOT, "!", startLine, startCol);
        case '?': return Token(TokenType::QUESTION, "?", startLine, startCol);
        case '(': return Token(TokenType::LPAREN, "(", startLine, startCol);
        case ')': return Token(TokenType::RPAREN, ")", startLine, startCol);
        case '{': return Token(TokenType::LBRACE, "{", startLine, startCol);
        case '}': return Token(TokenType::RBRACE, "}", startLine, startCol);
        case '[': return Token(TokenType::LBRACKET, "[", startLine, startCol);
        case ']': return Token(TokenType::RBRACKET, "]", startLine, startCol);
        case ',': return Token(TokenType::COMMA, ",", startLine, startCol);
        case ':': return Token(TokenType::COLON, ":", startLine, startCol);
        case ';': return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case '.': return Token(TokenType::DOT, ".", startLine, startCol);
        default:
            return Token(TokenType::ERR, std::string(1, c), startLine, startCol);
    }
}

std::vector<Token> Scanner::tokenize() {
    std::vector<Token> tokens;
    Token tok;
    do {
        tok = nextToken();
        tokens.push_back(tok);
    } while (tok.type != TokenType::END);
    return tokens;
}
