//
// Created by ashehalsadik on 4/24/26.
//

#include "grammar.h"

#include <exception>
#include <iostream>

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
        std::cerr << "Usage: " << argv[0] << " <grammar-file>\n";
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
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}