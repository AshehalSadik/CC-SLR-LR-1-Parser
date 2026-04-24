//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_PARSING_TABLE_H
#define CC_SLR_LR_1_PARSER_PARSING_TABLE_H

#include "grammar.h"
#include "items.h"

#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct ProductionRule {
	int index = -1;
	Grammar::Symbol lhs;
	Grammar::ProductionBody rhs;

	std::string toString() const;
};

enum class ActionType {
	Error,
	Shift,
	Reduce,
	Accept
};

struct ActionEntry {
	ActionType type = ActionType::Error;
	int targetState = -1;
	int productionIndex = -1;

	std::string toString() const;
	bool operator==(const ActionEntry &other) const;
};

struct Conflict {
	int state = -1;
	Grammar::Symbol symbol;
	ActionEntry existing;
	ActionEntry incoming;
	std::string kind;
};

class SLRParsingTable {
public:
	SLRParsingTable(const Grammar &grammar, const LR0Automaton &automaton);

	void build();
	void print(std::ostream &os) const;
	void printConflicts(std::ostream &os) const;

	ActionEntry getAction(int stateId, const Grammar::Symbol &terminal) const;
	std::optional<int> getGoto(int stateId, const Grammar::Symbol &nonTerminal) const;

	const std::vector<ProductionRule> &getProductionRules() const { return productionRules_; }
	const std::vector<Conflict> &getConflicts() const { return conflicts_; }
	bool isSLR1() const { return conflicts_.empty(); }

private:
	const Grammar &grammar_;
	const LR0Automaton &automaton_;

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

#endif //CC_SLR_LR_1_PARSER_PARSING_TABLE_H
