//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_ITEMS_H
#define CC_SLR_LR_1_PARSER_ITEMS_H

#include "grammar.h"

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct LR0Item {
	Grammar::Symbol lhs;
	Grammar::ProductionBody rhs;
	std::size_t dot = 0;

	bool operator<(const LR0Item &other) const;
	bool operator==(const LR0Item &other) const;
};

struct LR0State {
	int id = -1;
	std::set<LR0Item> items;
};

class LR0Automaton {
public:
	explicit LR0Automaton(const Grammar &grammar);

	void build();
	void print(std::ostream &os) const;

	const std::vector<LR0State> &getStates() const { return states_; }
	const std::map<std::pair<int, Grammar::Symbol>, int> &getTransitions() const { return transitions_; }
	std::optional<int> getTransition(int stateId, const Grammar::Symbol &symbol) const;

private:
	const Grammar &grammar_;
	std::vector<LR0State> states_;
	std::map<std::pair<int, Grammar::Symbol>, int> transitions_;

	std::set<LR0Item> closure(const std::set<LR0Item> &seed) const;
	std::set<LR0Item> goTo(const std::set<LR0Item> &stateItems, const Grammar::Symbol &symbol) const;
	int findExistingState(const std::set<LR0Item> &items) const;

	static Grammar::ProductionBody normalizeRhs(const Grammar::ProductionBody &rhs);
	static std::string itemToString(const LR0Item &item);
};

#endif //CC_SLR_LR_1_PARSER_ITEMS_H
