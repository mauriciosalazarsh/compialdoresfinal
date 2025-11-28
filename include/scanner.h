#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include <string>
#include <vector>

class Scanner {
private:
    std::string input;
    size_t pos;
    int line;
    int column;
    char current;

    void advance();
    char peek(int offset = 1);
    void skipWhitespace();
    void skipComment();
    void skipPreprocessor();

    Token scanNumber();
    Token scanIdentifier();
    Token scanString();

public:
    Scanner(const std::string& src);
    Token nextToken();
    std::vector<Token> tokenize();
};

#endif // SCANNER_H
