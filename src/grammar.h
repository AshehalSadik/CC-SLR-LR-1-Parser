//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_GRAMMAR_H
#define CC_SLR_LR_1_PARSER_GRAMMAR_H

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

class Grammar {
public:
	using Symbol = std::string;
	using ProductionBody = std::vector<Symbol>;
	using ProductionMap = std::map<Symbol, std::vector<ProductionBody>>;

	Grammar() = default;
	explicit Grammar(const std::string &grammarPath);

	void loadFromFile(const std::string &grammarPath);
	void augment();

	std::map<Symbol, std::set<Symbol>> computeFirstSets() const;
	std::map<Symbol, std::set<Symbol>> computeFollowSets(const std::map<Symbol, std::set<Symbol>> &firstSets) const;
	std::set<Symbol> firstOfSequence(const ProductionBody &sequence,
									 const std::map<Symbol, std::set<Symbol>> &firstSets) const;

	void print(std::ostream &os) const;

	const ProductionMap &getProductions() const { return productions_; }
	const std::set<Symbol> &getNonTerminals() const { return nonTerminals_; }
	const std::set<Symbol> &getTerminals() const { return terminals_; }
	const Symbol &getStartSymbol() const { return startSymbol_; }
	const Symbol &getAugmentedStartSymbol() const { return augmentedStartSymbol_; }

	static const Symbol kEpsilon;
	static const Symbol kEndMarker;

private:
	ProductionMap productions_;
	std::set<Symbol> nonTerminals_;
	std::set<Symbol> terminals_;
	Symbol startSymbol_;
	Symbol augmentedStartSymbol_;

	void recomputeTerminals();
	static bool isInputNonTerminal(const Symbol &symbol);
	static ProductionBody splitSymbols(const std::string &text);
	static std::string trim(const std::string &value);
};


#endif //CC_SLR_LR_1_PARSER_GRAMMAR_H
