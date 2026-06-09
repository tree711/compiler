#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Semantic.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

struct SemanticTestCase {
    std::string path;
    bool shouldPass;
    std::string expectedError;
};

static bool runSemanticTest(const SemanticTestCase& test) {
    try {
        Lexer lexer(test.path);
        Parser parser(lexer);
        ASTNodePtr ast = parser.parse();

        SemanticAnalyzer analyzer;
        analyzer.analyze(ast.get());

        if (!test.shouldPass) {
            std::cerr << "FAIL " << test.path
                      << ": expected semantic error but passed\n";
            return false;
        }

        std::cout << "PASS " << test.path << "\n";
        return true;
    } catch (const std::exception& error) {
        const std::string message = error.what();
        if (test.shouldPass) {
            std::cerr << "FAIL " << test.path << ": " << message << "\n";
            return false;
        }
        if (!test.expectedError.empty() &&
            message.find(test.expectedError) == std::string::npos) {
            std::cerr << "FAIL " << test.path
                      << ": unexpected error: " << message << "\n";
            return false;
        }
        std::cout << "PASS " << test.path << " (expected error)\n";
        return true;
    }
}

static int runSemanticTests() {
    const std::vector<SemanticTestCase> tests = {
        {"tests/semantic_valid.tc", true, ""},
        {"tests/semantic_undefined.tc", false, "is not declared"},
        {"tests/semantic_duplicate.tc", false, "already declared"},
        {"tests/semantic_scope.tc", false, "is not declared"},
        {"tests/semantic_self_init.tc", false, "is not declared"},
        {"tests/semantic_shadow.tc", false, "already declared"},
    };

    int failed = 0;
    for (const auto& test : tests) {
        if (!runSemanticTest(test)) {
            ++failed;
        }
    }

    if (failed == 0) {
        std::cout << "All semantic tests passed.\n";
    } else {
        std::cerr << failed << " semantic test(s) failed.\n";
    }
    return failed == 0 ? 0 : 1;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return runSemanticTests();
    }

    if (argc != 2) {
        std::cerr << "Usage:\n"
                  << "  semantic_test                 Run built-in semantic tests\n"
                  << "  semantic_test <input.tc>      Analyze one source file\n";
        return 1;
    }

    try {
        Lexer lexer(argv[1]);
        Parser parser(lexer);
        ASTNodePtr ast = parser.parse();

        SemanticAnalyzer analyzer;
        analyzer.analyze(ast.get());

        std::cout << "Semantic Pass\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }

    return 0;
}
