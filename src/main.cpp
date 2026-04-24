//
// Created by ashehalsadik on 4/24/26.
//

#include "grammar.h"
#include "items.h"
#include "lr1_parser.h"
#include "parsing_table.h"
#include "slr_parser.h"
#include "tree.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <sstream>

enum class ParserMode {
  Both,
  SLR,
  LR1
};

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

template <typename Func>
static double measureMs(Func &&func) {
  const auto start = std::chrono::steady_clock::now();
  func();
  const auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <grammar-file> [--mode slr|lr1|both] [input tokens]\n";
        std::cerr << "Examples:\n";
        std::cerr << "  " << argv[0] << " input/grammar.txt\n";
        std::cerr << "  " << argv[0] << " input/grammar.txt --mode slr \"id + id * id\"\n";
        std::cerr << "  " << argv[0] << " input/grammar.txt --mode lr1 \"id = id\"\n";
        return 1;
    }

    try {
        ParserMode mode = ParserMode::Both;
        std::vector<std::string> rawInputArgs;
        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--mode") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("Missing value for --mode. Expected: slr, lr1, or both.");
                }
                const std::string modeValue = argv[++i];
                if (modeValue == "slr") {
                    mode = ParserMode::SLR;
                } else if (modeValue == "lr1") {
                    mode = ParserMode::LR1;
                } else if (modeValue == "both") {
                    mode = ParserMode::Both;
                } else {
                    throw std::runtime_error("Invalid mode '" + modeValue + "'. Use slr, lr1, or both.");
                }
                continue;
            }
            rawInputArgs.push_back(arg);
        }

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
        const double lr0BuildMs = measureMs([&automaton]() {
            automaton.build();
        });

        std::cout << "Canonical Collection of LR(0) Items:\n";
        automaton.print(std::cout);

        SLRParsingTable table(grammar, automaton);
        const double slrTableBuildMs = measureMs([&table]() {
            table.build();
        });
        table.print(std::cout);
        table.printConflicts(std::cout);

        LR1Automaton lr1Automaton(grammar);
        const double lr1AutomatonBuildMs = measureMs([&lr1Automaton]() {
            lr1Automaton.build();
        });

        std::cout << "Canonical Collection of LR(1) Items:\n";
        lr1Automaton.print(std::cout);

        LR1ParsingTable lr1Table(grammar, lr1Automaton);
        const double lr1TableBuildMs = measureMs([&lr1Table]() {
            lr1Table.build();
        });
        lr1Table.print(std::cout);
        lr1Table.printConflicts(std::cout);

        std::cout << "State count comparison: SLR(LR(0) DFA)=" << automaton.getStates().size()
                  << ", LR(1)=" << lr1Automaton.getStates().size() << "\n\n";

        std::cout << "Performance Summary:\n";
        std::cout << "  SLR states: " << automaton.getStates().size()
                  << ", transitions: " << automaton.getTransitions().size() << '\n';
        std::cout << "  SLR table entries: ACTION=" << table.actionEntryCount()
                  << ", GOTO=" << table.gotoEntryCount()
                  << ", estimated-bytes=" << table.estimatedTableBytes() << '\n';
        std::cout << "  SLR build time (ms): automaton=" << lr0BuildMs
                  << ", table=" << slrTableBuildMs << '\n';
        std::cout << "  LR(1) states: " << lr1Automaton.getStates().size()
                  << ", transitions: " << lr1Automaton.getTransitions().size() << '\n';
        std::cout << "  LR(1) table entries: ACTION=" << lr1Table.actionEntryCount()
                  << ", GOTO=" << lr1Table.gotoEntryCount()
                  << ", estimated-bytes=" << lr1Table.estimatedTableBytes() << '\n';
        std::cout << "  LR(1) build time (ms): automaton=" << lr1AutomatonBuildMs
                  << ", table=" << lr1TableBuildMs << "\n\n";

        if (!rawInputArgs.empty()) {
            std::vector<Grammar::Symbol> inputTokens;
            if (rawInputArgs.size() == 1) {
                inputTokens = SLRParser::splitInput(rawInputArgs.front());
            } else {
                for (const auto &token : rawInputArgs) {
                    inputTokens.push_back(token);
                }
            }

            if (mode == ParserMode::Both || mode == ParserMode::SLR) {
                if (!table.isSLR1()) {
                    std::cout << "Skipping SLR parsing because grammar has SLR conflicts.\n";
                } else {
                const SLRParser parser(grammar, table);
                const ParseResult result = parser.parse(inputTokens);
                SLRParser::printTrace(result, std::cout);

                constexpr int kBenchmarkIterations = 2000;
                const double slrParseMs = measureMs([&parser, &inputTokens]() {
                    for (int i = 0; i < kBenchmarkIterations; ++i) {
                        (void) parser.parse(inputTokens);
                    }
                });
                std::cout << "SLR parse benchmark: " << kBenchmarkIterations << " runs in "
                          << slrParseMs << " ms (avg " << (slrParseMs / kBenchmarkIterations)
                          << " ms/run)\n\n";

                if (result.accepted) {
                    std::cout << "SLR Parse Tree:\n";
                    printParseTree(result.parseTreeRoot, std::cout);
                    std::cout << '\n';
                }
                }
            }

            if (mode == ParserMode::Both || mode == ParserMode::LR1) {
                if (!lr1Table.isLR1()) {
                    std::cout << "Skipping LR(1) parsing because grammar has LR(1) conflicts.\n";
                    return 0;
                }

                const LR1Parser lr1Parser(grammar, lr1Table);
                const LR1ParseResult lr1Result = lr1Parser.parse(inputTokens);
                LR1Parser::printTrace(lr1Result, std::cout);

                constexpr int kBenchmarkIterations = 2000;
                const double lr1ParseMs = measureMs([&lr1Parser, &inputTokens]() {
                    for (int i = 0; i < kBenchmarkIterations; ++i) {
                        (void) lr1Parser.parse(inputTokens);
                    }
                });
                std::cout << "LR(1) parse benchmark: " << kBenchmarkIterations << " runs in "
                          << lr1ParseMs << " ms (avg " << (lr1ParseMs / kBenchmarkIterations)
                          << " ms/run)\n\n";

                if (lr1Result.accepted) {
                    std::cout << "LR(1) Parse Tree:\n";
                    printParseTree(lr1Result.parseTreeRoot, std::cout);
                }
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}