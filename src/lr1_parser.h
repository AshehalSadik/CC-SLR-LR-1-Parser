//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_LR1_PARSER_H
#define CC_SLR_LR_1_PARSER_LR1_PARSER_H

#include "grammar.h"
#include "parsing_table.h"
#include "tree.h"

#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct LR1Item {
	Grammar::Symbol lhs;
	Grammar::ProductionBody rhs;
	std::size_t dot = 0;
	Grammar::Symbol lookahead;

	bool operator<(const LR1Item &other) const;
	bool operator==(const LR1Item &other) const;
};

struct LR1State {
	int id = -1;
	std::set<LR1Item> items;
};

class LR1Automaton {
public:
	explicit LR1Automaton(const Grammar &grammar);

	void build();
	void print(std::ostream &os) const;

	const std::vector<LR1State> &getStates() const { return states_; }
	const std::map<std::pair<int, Grammar::Symbol>, int> &getTransitions() const { return transitions_; }
	std::optional<int> getTransition(int stateId, const Grammar::Symbol &symbol) const;

private:
	const Grammar &grammar_;
	std::vector<LR1State> states_;
	std::map<std::pair<int, Grammar::Symbol>, int> transitions_;

	std::set<LR1Item> closure(const std::set<LR1Item> &seed,
							  const std::map<Grammar::Symbol, std::set<Grammar::Symbol>> &firstSets) const;
	std::set<LR1Item> goTo(const std::set<LR1Item> &stateItems,
						  const Grammar::Symbol &symbol,
						  const std::map<Grammar::Symbol, std::set<Grammar::Symbol>> &firstSets) const;
	int findExistingState(const std::set<LR1Item> &items) const;

	static Grammar::ProductionBody normalizeRhs(const Grammar::ProductionBody &rhs);
	static std::string itemToString(const LR1Item &item);
};

class LR1ParsingTable {
public:
	LR1ParsingTable(const Grammar &grammar, const LR1Automaton &automaton);

	void build();
	void print(std::ostream &os) const;
	void printConflicts(std::ostream &os) const;

	ActionEntry getAction(int stateId, const Grammar::Symbol &terminal) const;
	std::optional<int> getGoto(int stateId, const Grammar::Symbol &nonTerminal) const;

	const std::vector<ProductionRule> &getProductionRules() const { return productionRules_; }
	const std::vector<Conflict> &getConflicts() const { return conflicts_; }
	bool isLR1() const { return conflicts_.empty(); }
	std::size_t actionEntryCount() const;
	std::size_t gotoEntryCount() const;
	std::size_t estimatedTableBytes() const;

private:
	const Grammar &grammar_;
	const LR1Automaton &automaton_;

	std::vector<ProductionRule> productionRules_;
	std::map<std::pair<Grammar::Symbol, Grammar::ProductionBody>, int> productionIndexByBody_;
	std::map<std::pair<int, Grammar::Symbol>, ActionEntry> actionTable_;
	std::map<std::pair<int, Grammar::Symbol>, int> gotoTable_;
	std::vector<Conflict> conflicts_;

	void collectProductionRules();
	int findProductionIndex(const Grammar::Symbol &lhs, const Grammar::ProductionBody &rhs) const;
	void setAction(int stateId, const Grammar::Symbol &terminal, const ActionEntry &action);

	static Grammar::ProductionBody normalizeRhs(const Grammar::ProductionBody &rhs);
	static std::string classifyConflict(const ActionEntry &existing, const ActionEntry &incoming);
};

struct LR1ParseStep {
	int step = 0;
	std::string stack;
	std::string input;
	std::string action;
};

struct LR1ParseResult {
	bool accepted = false;
	std::string errorMessage;
	std::vector<LR1ParseStep> trace;
	std::shared_ptr<ParseTreeNode> parseTreeRoot;
};

class LR1Parser {
public:
	LR1Parser(const Grammar &grammar, const LR1ParsingTable &table);

	LR1ParseResult parse(const std::vector<Grammar::Symbol> &inputTokens) const;

	static std::vector<Grammar::Symbol> splitInput(const std::string &inputText);
	static void printTrace(const LR1ParseResult &result, std::ostream &os);

private:
	const Grammar &grammar_;
	const LR1ParsingTable &table_;

	static std::string formatStack(const std::vector<int> &states, const std::vector<Grammar::Symbol> &symbols);
	static std::string formatRemainingInput(const std::vector<Grammar::Symbol> &tokens, std::size_t index);
};


#endif //CC_SLR_LR_1_PARSER_LR1_PARSER_H
