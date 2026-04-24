//
// Created by ashehalsadik on 4/24/26.
//

#include "grammar.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

const Grammar::Symbol Grammar::kEpsilon = "epsilon";
const Grammar::Symbol Grammar::kEndMarker = "$";

Grammar::Grammar(const std::string &grammarPath) {
	loadFromFile(grammarPath);
}

void Grammar::loadFromFile(const std::string &grammarPath) {
	productions_.clear();
	nonTerminals_.clear();
	terminals_.clear();
	startSymbol_.clear();
	augmentedStartSymbol_.clear();

	std::ifstream input(grammarPath);
	if (!input.is_open()) {
		throw std::runtime_error("Failed to open grammar file: " + grammarPath);
	}

	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(input, line)) {
		++lineNumber;
		const std::string raw = trim(line);
		if (raw.empty()) {
			continue;
		}

		const auto arrowPos = raw.find("->");
		if (arrowPos == std::string::npos) {
			throw std::runtime_error("Missing '->' at line " + std::to_string(lineNumber));
		}

		const Symbol lhs = trim(raw.substr(0, arrowPos));
		if (!isInputNonTerminal(lhs)) {
			throw std::runtime_error(
				"Invalid non-terminal '" + lhs + "' at line " + std::to_string(lineNumber) +
				". Non-terminals must start with uppercase and be at least 2 characters.");
		}

		if (startSymbol_.empty()) {
			startSymbol_ = lhs;
		}
		nonTerminals_.insert(lhs);

		const std::string rhsText = raw.substr(arrowPos + 2);
		std::stringstream rhsStream(rhsText);
		std::string alternative;
		while (std::getline(rhsStream, alternative, '|')) {
			ProductionBody symbols = splitSymbols(trim(alternative));
			if (symbols.empty()) {
				throw std::runtime_error("Empty production alternative at line " + std::to_string(lineNumber));
			}

			for (Symbol &symbol : symbols) {
				if (symbol == "@") {
					symbol = kEpsilon;
				}
			}
			productions_[lhs].push_back(std::move(symbols));
		}

		if (productions_[lhs].empty()) {
			throw std::runtime_error("No productions found for " + lhs + " at line " + std::to_string(lineNumber));
		}
	}

	if (productions_.empty()) {
		throw std::runtime_error("Grammar file is empty: " + grammarPath);
	}

	recomputeTerminals();
}

void Grammar::augment() {
	if (startSymbol_.empty()) {
		throw std::runtime_error("Cannot augment an empty grammar.");
	}

	if (!augmentedStartSymbol_.empty()) {
		return;
	}

	Symbol candidate = startSymbol_ + "'";
	while (nonTerminals_.count(candidate) != 0U) {
		candidate.push_back('\'');
	}

	augmentedStartSymbol_ = candidate;
	nonTerminals_.insert(augmentedStartSymbol_);
	productions_[augmentedStartSymbol_].push_back({startSymbol_});

	recomputeTerminals();
}

std::map<Grammar::Symbol, std::set<Grammar::Symbol>> Grammar::computeFirstSets() const {
	std::map<Symbol, std::set<Symbol>> first;
	for (const auto &nt : nonTerminals_) {
		first[nt] = {};
	}

	bool changed = true;
	while (changed) {
		changed = false;
		for (const auto &[lhs, rhsList] : productions_) {
			for (const auto &rhs : rhsList) {
				bool allNullable = true;
				for (const auto &symbol : rhs) {
					if (symbol == kEpsilon) {
						if (first[lhs].insert(kEpsilon).second) {
							changed = true;
						}
						continue;
					}

					if (terminals_.count(symbol) != 0U) {
						if (first[lhs].insert(symbol).second) {
							changed = true;
						}
						allNullable = false;
						break;
					}

					if (nonTerminals_.count(symbol) != 0U) {
						bool symbolNullable = false;
						for (const auto &f : first[symbol]) {
							if (f == kEpsilon) {
								symbolNullable = true;
							} else if (first[lhs].insert(f).second) {
								changed = true;
							}
						}

						if (!symbolNullable) {
							allNullable = false;
							break;
						}
						continue;
					}

					throw std::runtime_error("Unknown symbol in production: " + symbol);
				}

				if (allNullable) {
					if (first[lhs].insert(kEpsilon).second) {
						changed = true;
					}
				}
			}
		}
	}

	return first;
}

std::set<Grammar::Symbol> Grammar::firstOfSequence(
	const ProductionBody &sequence,
	const std::map<Symbol, std::set<Symbol>> &firstSets) const {
	std::set<Symbol> result;
	bool allNullable = true;

	for (const auto &symbol : sequence) {
		if (symbol == kEpsilon) {
			result.insert(kEpsilon);
			continue;
		}

		if (terminals_.count(symbol) != 0U || symbol == kEndMarker) {
			result.insert(symbol);
			allNullable = false;
			break;
		}

		if (nonTerminals_.count(symbol) != 0U) {
			bool nullable = false;
			const auto it = firstSets.find(symbol);
			if (it == firstSets.end()) {
				throw std::runtime_error("FIRST set missing non-terminal: " + symbol);
			}

			for (const auto &f : it->second) {
				if (f == kEpsilon) {
					nullable = true;
				} else {
					result.insert(f);
				}
			}

			if (!nullable) {
				allNullable = false;
				break;
			}
			continue;
		}

		throw std::runtime_error("Unknown symbol in FIRST(sequence): " + symbol);
	}

	if (allNullable) {
		result.insert(kEpsilon);
	}
	return result;
}

std::map<Grammar::Symbol, std::set<Grammar::Symbol>> Grammar::computeFollowSets(
	const std::map<Symbol, std::set<Symbol>> &firstSets) const {
	std::map<Symbol, std::set<Symbol>> follow;
	for (const auto &nt : nonTerminals_) {
		follow[nt] = {};
	}

	if (!augmentedStartSymbol_.empty()) {
		follow[augmentedStartSymbol_].insert(kEndMarker);
	} else if (!startSymbol_.empty()) {
		follow[startSymbol_].insert(kEndMarker);
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (const auto &[lhs, rhsList] : productions_) {
			for (const auto &rhs : rhsList) {
				for (std::size_t i = 0; i < rhs.size(); ++i) {
					const Symbol &current = rhs[i];
					if (nonTerminals_.count(current) == 0U) {
						continue;
					}

					ProductionBody beta;
					if (i + 1 < rhs.size()) {
						beta.assign(rhs.begin() + (i + 1), rhs.end());
					}

					const auto firstBeta = firstOfSequence(beta, firstSets);
					for (const auto &f : firstBeta) {
						if (f != kEpsilon && follow[current].insert(f).second) {
							changed = true;
						}
					}

					if (beta.empty() || firstBeta.count(kEpsilon) != 0U) {
						for (const auto &f : follow[lhs]) {
							if (follow[current].insert(f).second) {
								changed = true;
							}
						}
					}
				}
			}
		}
	}

	return follow;
}

void Grammar::print(std::ostream &os) const {
	for (const auto &[lhs, rhsList] : productions_) {
		os << lhs << " -> ";
		for (std::size_t i = 0; i < rhsList.size(); ++i) {
			for (const auto &symbol : rhsList[i]) {
				os << symbol << ' ';
			}
			if (i + 1 < rhsList.size()) {
				os << "| ";
			}
		}
		os << '\n';
	}
}

void Grammar::recomputeTerminals() {
	terminals_.clear();
	for (const auto &[lhs, rhsList] : productions_) {
		(void) lhs;
		for (const auto &rhs : rhsList) {
			for (const auto &symbol : rhs) {
				if (symbol == kEpsilon) {
					continue;
				}

				if (nonTerminals_.count(symbol) == 0U) {
					terminals_.insert(symbol);
				}
			}
		}
	}
}

bool Grammar::isInputNonTerminal(const Symbol &symbol) {
	if (symbol.size() < 2) {
		return false;
	}
	if (!std::isupper(static_cast<unsigned char>(symbol[0]))) {
		return false;
	}
	return std::all_of(symbol.begin() + 1, symbol.end(), [](const unsigned char c) {
		return std::isalnum(c) != 0 || c == '_';
	});
}

Grammar::ProductionBody Grammar::splitSymbols(const std::string &text) {
	ProductionBody symbols;
	std::stringstream ss(text);
	Symbol token;
	while (ss >> token) {
		symbols.push_back(token);
	}
	return symbols;
}

std::string Grammar::trim(const std::string &value) {
	std::size_t left = 0;
	while (left < value.size() && std::isspace(static_cast<unsigned char>(value[left])) != 0) {
		++left;
	}

	std::size_t right = value.size();
	while (right > left && std::isspace(static_cast<unsigned char>(value[right - 1])) != 0) {
		--right;
	}
	return value.substr(left, right - left);
}

