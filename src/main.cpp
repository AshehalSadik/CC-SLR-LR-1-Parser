//
// Created by ashehalsadik on 4/24/26.
//

#include "grammar.h"
#include "items.h"
#include "parsing_table.h"
#include "slr_parser.h"
#include "tree.h"

#include <exception>
#include <iostream>
#include <sstream>

static void printSets(const std::map<std::string, std::set<std::string>> &sets, const std::string &label) {
    std::cout << label << ":\n";
    for (const auto &[symbol, values] : sets) {
        std::cout << "  " << symbol << " = { ";
        bool first = true;
        for (const auto &value : values) {
            if (!first) {
                std::cout << ", ";
            }
            std::cout << value;
            first = false;
        }
        std::cout << " }\n";
    }
    std::cout << '\n';
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <grammar-file> [input tokens]\n";
        std::cerr << "Examples:\n";
        std::cerr << "  " << argv[0] << " input/grammar.txt\n";
        std::cerr << "  " << argv[0] << " input/grammar.txt \"id + id * id\"\n";
        return 1;
    }

    try {
        Grammar grammar(argv[1]);
        grammar.augment();

        std::cout << "Augmented Grammar:\n";
        grammar.print(std::cout);
        std::cout << '\n';

        const auto firstSets = grammar.computeFirstSets();
        const auto followSets = grammar.computeFollowSets(firstSets);

        printSets(firstSets, "FIRST");
        printSets(followSets, "FOLLOW");

        LR0Automaton automaton(grammar);
        automaton.build();

        std::cout << "Canonical Collection of LR(0) Items:\n";
        automaton.print(std::cout);

        SLRParsingTable table(grammar, automaton);
        table.build();
        table.print(std::cout);
        table.printConflicts(std::cout);

        if (argc >= 3) {
            std::vector<Grammar::Symbol> inputTokens;
            if (argc == 3) {
                inputTokens = SLRParser::splitInput(argv[2]);
            } else {
                for (int i = 2; i < argc; ++i) {
                    inputTokens.push_back(argv[i]);
                }
            }

            if (!table.isSLR1()) {
                std::cout << "Skipping parsing because grammar has SLR conflicts.\n";
                return 0;
            }

            const SLRParser parser(grammar, table);
            const ParseResult result = parser.parse(inputTokens);
            SLRParser::printTrace(result, std::cout);

            if (result.accepted) {
                std::cout << "Parse Tree:\n";
                printParseTree(result.parseTreeRoot, std::cout);
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}