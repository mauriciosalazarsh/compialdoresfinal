#include <iostream>
#include <fstream>
#include <sstream>
#include "../include/scanner.h"
#include "../include/parser.h"
#include "../include/semantic.h"
#include "../include/codegen.h"

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not write to file: " + filename);
    }
    file << content;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.kt> [-o output.s]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = "output.s";

    // opciones de linea de comandos
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
    }

    try {
        // leer archivo fuente
        std::cout << "Reading source file: " << inputFile << std::endl;
        std::string source = readFile(inputFile);

        // analisis lexico
        std::cout << "Performing lexical analysis..." << std::endl;
        Scanner scanner(source);
        std::vector<Token> tokens = scanner.tokenize();

        std::cout << "Tokens generated: " << tokens.size() << std::endl;

        // analisis sintactico
        std::cout << "Performing syntax analysis..." << std::endl;
        Parser parser(tokens);
        auto program = parser.parse();

        std::cout << "Syntax analysis completed successfully." << std::endl;

        // analisis semantico
        std::cout << "Performing semantic analysis..." << std::endl;
        SymbolTable symbolTable;
        SemanticAnalyzer semantic(symbolTable);
        program->accept(&semantic);

        if (semantic.hasError()) {
            std::cerr << "Semantic errors found:\n" << semantic.getErrors();
            return 1;
        }

        std::cout << "Semantic analysis completed successfully." << std::endl;

        // generacion de codigo
        std::cout << "Generating x86-64 assembly code..." << std::endl;
        SymbolTable codegenSymbolTable;
        CodeGenerator codegen(codegenSymbolTable);
        codegen.enableOptimizations(true, true);
        program->accept(&codegen);

        std::string assemblyCode = codegen.getCode();

        // escribir salida
        std::cout << "Writing assembly to: " << outputFile << std::endl;
        writeFile(outputFile, assemblyCode);

        std::cout << "\nCompilation successful!" << std::endl;
        std::cout << "Assembly file generated: " << outputFile << std::endl;
        std::cout << "\nTo assemble and run:" << std::endl;
        std::cout << "  gcc -no-pie " << outputFile << " -o program" << std::endl;
        std::cout << "  ./program" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
