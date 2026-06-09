#include "../include/CodeGenerator.h"
#include "../include/IRGenerator.h"
#include "../include/Optimizer.h"
#include "../include/Parser.h"
#include "../include/Semantic.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: compiler <input.c> [output.s]\n";
        return 1;
    }

    const std::string outputPath = argc == 3 ? argv[2] : "output.s";

    try {
        Lexer lexer(argv[1]);
        Parser parser(lexer);
        ASTNodePtr ast = parser.parse();

        SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(ast.get());

        IRGenerator irGenerator;
        IRProgram program = irGenerator.generate(ast.get());

        Optimizer optimizer;
        optimizer.optimize(program);

        CodeGenerator codeGenerator(outputPath);
        codeGenerator.generate(program);
        std::cout << "Compiled " << argv[1] << " -> " << outputPath << "\n";
    } catch (const std::exception& error) {
        std::cerr << "Compilation failed: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
