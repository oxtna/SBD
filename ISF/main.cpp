#include "engine.h"
#include "lexer.h"
#include "parser.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

static std::string read_file(const char* filename) {
    std::ifstream source(filename);
    std::ostringstream buffer{};
    buffer << source.rdbuf();
    return buffer.str();
}

static std::istream& prompt_and_read_input(
    std::string& destination, std::ostream& output = std::cout, std::istream& input = std::cin) {
    output << "> ";
    return std::getline(input, destination);
}

int main() {
    std::string line{};
    Lexer lexer(std::regex(R"(\b(show|insert|update|remove|list|sort|status)\b|[+-]?\d+|;|,|\{|\})"));
    Parser parser{};
    Engine engine(std::cout);
    while (prompt_and_read_input(line)) {
        if (line.empty()) {
            continue;
        }
        auto tokens = lexer.tokenize(line);
        auto statements = parser.parse(tokens);
        for (const auto& statement : statements) {
            try {
                engine.execute(statement.get());
            }
            catch (const NotImplementedError& e) {
                std::cout << e.what() << '\n';
            }
            catch (const InvalidStatementError& e) {
                std::cout << e.what() << '\n';
            }
            catch (const NotInitializedError& e) {
                std::cout << e.what() << '\n';
            }
            catch (const KeyError& e) {
                std::cout << e.what() << '\n';
            }
        }
    }
    return 0;
}
