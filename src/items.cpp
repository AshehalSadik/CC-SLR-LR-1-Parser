//
// Created by ashehalsadik on 4/24/26.
//

#include "items.h"

#include <queue>
#include <sstream>
#include <stdexcept>

bool LR0Item::operator<(const LR0Item &other) const {
	if (lhs != other.lhs) {
		return lhs < other.lhs;
	}
	if (rhs != other.rhs) {
		return rhs < other.rhs;
	}
	return dot < other.dot;
}

bool LR0Item::operator==(const LR0Item &other) const {
	return lhs == other.lhs && rhs == other.rhs && dot == other.dot;
}

LR0Automaton::LR0Automaton(const Grammar &grammar) : grammar_(grammar) {
}

void LR0Automaton::build() {
	states_.clear();
	transitions_.clear();

	const auto &productions = grammar_.getProductions();
	const auto &augmentedStart = grammar_.getAugmentedStartSymbol();
	if (augmentedStart.empty()) {
		throw std::runtime_error("Grammar must be augmented before building LR(0) items.");
	}

	const auto startIt = productions.find(augmentedStart);
	if (startIt == productions.end() || startIt->second.empty()) {
		throw std::runtime_error("Augmented start production not found.");
	}

	const LR0Item startItem{augmentedStart, normalizeRhs(startIt->second.front()), 0};
	LR0State initial;
	initial.id = 0;
	initial.items = closure({startItem});
	states_.push_back(initial);

	std::set<Grammar::Symbol> symbols;
	for (const auto &terminal : grammar_.getTerminals()) {
		symbols.insert(terminal);
	}
	for (const auto &nonTerminal : grammar_.getNonTerminals()) {
		symbols.insert(nonTerminal);
	}

	std::queue<int> pending;
	pending.push(0);

	while (!pending.empty()) {
		const int currentStateId = pending.front();
		pending.pop();

		for (const auto &symbol : symbols) {
			const auto targetItems = goTo(states_[static_cast<std::size_t>(currentStateId)].items, symbol);
			if (targetItems.empty()) {
				continue;
			}

			int existing = findExistingState(targetItems);
			if (existing < 0) {
				LR0State newState;
				newState.id = static_cast<int>(states_.size());
				newState.items = targetItems;
				states_.push_back(newState);
				existing = newState.id;
				pending.push(existing);
			}

			transitions_[{currentStateId, symbol}] = existing;
		}
	}
}

std::optional<int> LR0Automaton::getTransition(const int stateId, const Grammar::Symbol &symbol) const {
	const auto it = transitions_.find({stateId, symbol});
	if (it == transitions_.end()) {
		return std::nullopt;
	}
	return it->second;
}

void LR0Automaton::print(std::ostream &os) const {
	for (const auto &state : states_) {
		os << "I" << state.id << ":\n";
		for (const auto &item : state.items) {
			os << "  " << itemToString(item) << '\n';
		}
		os << '\n';
	}

	os << "Transitions:\n";
	for (const auto &[key, target] : transitions_) {
		os << "  GOTO(I" << key.first << ", " << key.second << ") = I" << target << '\n';
	}
	os << '\n';
}

std::set<LR0Item> LR0Automaton::closure(const std::set<LR0Item> &seed) const {
	std::set<LR0Item> result = seed;
	bool changed = true;

	while (changed) {
		changed = false;
		std::vector<LR0Item> toAdd;

		for (const auto &item : result) {
			if (item.dot >= item.rhs.size()) {
				continue;
			}

			const auto &symbolAfterDot = item.rhs[item.dot];
			if (grammar_.getNonTerminals().count(symbolAfterDot) == 0U) {
				continue;
			}

			const auto prodIt = grammar_.getProductions().find(symbolAfterDot);
			if (prodIt == grammar_.getProductions().end()) {
				throw std::runtime_error("Missing productions for non-terminal: " + symbolAfterDot);
			}

			for (const auto &rhs : prodIt->second) {
				toAdd.push_back({symbolAfterDot, normalizeRhs(rhs), 0});
			}
		}

		for (const auto &candidate : toAdd) {
			if (result.insert(candidate).second) {
				changed = true;
			}
		}
	}

	return result;
}

std::set<LR0Item> LR0Automaton::goTo(const std::set<LR0Item> &stateItems, const Grammar::Symbol &symbol) const {
	std::set<LR0Item> moved;
	for (const auto &item : stateItems) {
		if (item.dot < item.rhs.size() && item.rhs[item.dot] == symbol) {
			LR0Item advanced = item;
			++advanced.dot;
			moved.insert(advanced);
		}
	}

	if (moved.empty()) {
		return {};
	}
	return closure(moved);
}

int LR0Automaton::findExistingState(const std::set<LR0Item> &items) const {
	for (const auto &state : states_) {
		if (state.items == items) {
			return state.id;
		}
	}
	return -1;
}

Grammar::ProductionBody LR0Automaton::normalizeRhs(const Grammar::ProductionBody &rhs) {
	if (rhs.size() == 1 && rhs.front() == Grammar::kEpsilon) {
		return {};
	}
	return rhs;
}

std::string LR0Automaton::itemToString(const LR0Item &item) {
	std::ostringstream oss;
	oss << item.lhs << " -> ";

	if (item.rhs.empty()) {
		if (item.dot == 0) {
			oss << ". ";
		}
		oss << Grammar::kEpsilon;
		if (item.dot == 1) {
			oss << " .";
		}
		return oss.str();
	}

	for (std::size_t i = 0; i <= item.rhs.size(); ++i) {
		if (i == item.dot) {
			oss << ". ";
		}
		if (i < item.rhs.size()) {
			oss << item.rhs[i] << ' ';
		}
	}
	return oss.str();
}

