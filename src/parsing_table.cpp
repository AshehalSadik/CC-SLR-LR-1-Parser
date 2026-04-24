//
// Created by ashehalsadik on 4/24/26.
//

#include "parsing_table.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string ProductionRule::toString() const {
	std::ostringstream oss;
	oss << lhs << " -> ";
	if (rhs.empty()) {
		oss << Grammar::kEpsilon;
		return oss.str();
	}
	for (std::size_t i = 0; i < rhs.size(); ++i) {
		oss << rhs[i];
		if (i + 1 < rhs.size()) {
			oss << ' ';
		}
	}
	return oss.str();
}

std::string ActionEntry::toString() const {
	switch (type) {
		case ActionType::Shift:
			return "s" + std::to_string(targetState);
		case ActionType::Reduce:
			return "r" + std::to_string(productionIndex);
		case ActionType::Accept:
			return "acc";
		case ActionType::Error:
		default:
			return "";
	}
}

bool ActionEntry::operator==(const ActionEntry &other) const {
	return type == other.type && targetState == other.targetState && productionIndex == other.productionIndex;
}

SLRParsingTable::SLRParsingTable(const Grammar &grammar, const LR0Automaton &automaton)
	: grammar_(grammar), automaton_(automaton) {
}

void SLRParsingTable::build() {
	actionTable_.clear();
	gotoTable_.clear();
	conflicts_.clear();
	productionRules_.clear();
	productionIndexByBody_.clear();

	collectProductionRules();

	const auto firstSets = grammar_.computeFirstSets();
	const auto followSets = grammar_.computeFollowSets(firstSets);
	const auto &states = automaton_.getStates();

	for (const auto &state : states) {
		for (const auto &item : state.items) {
			if (item.dot < item.rhs.size()) {
				const auto &nextSymbol = item.rhs[item.dot];
				if (grammar_.getTerminals().count(nextSymbol) != 0U) {
					const auto transition = automaton_.getTransition(state.id, nextSymbol);
					if (transition.has_value()) {
						setAction(state.id, nextSymbol, {ActionType::Shift, *transition, -1});
					}
				}
				continue;
			}

			const bool isAugmentedAcceptItem =
				item.lhs == grammar_.getAugmentedStartSymbol() &&
				item.rhs.size() == 1 &&
				item.rhs.front() == grammar_.getStartSymbol();

			if (isAugmentedAcceptItem) {
				setAction(state.id, Grammar::kEndMarker, {ActionType::Accept, -1, -1});
				continue;
			}

			const int productionIndex = findProductionIndex(item.lhs, item.rhs);
			const auto followIt = followSets.find(item.lhs);
			if (followIt == followSets.end()) {
				throw std::runtime_error("FOLLOW set missing for non-terminal: " + item.lhs);
			}

			for (const auto &terminal : followIt->second) {
				setAction(state.id, terminal, {ActionType::Reduce, -1, productionIndex});
			}
		}

		for (const auto &nonTerminal : grammar_.getNonTerminals()) {
			if (nonTerminal == grammar_.getAugmentedStartSymbol()) {
				continue;
			}
			const auto transition = automaton_.getTransition(state.id, nonTerminal);
			if (transition.has_value()) {
				gotoTable_[{state.id, nonTerminal}] = *transition;
			}
		}
	}
}

void SLRParsingTable::print(std::ostream &os) const {
	std::vector<Grammar::Symbol> actionColumns;
	for (const auto &terminal : grammar_.getTerminals()) {
		actionColumns.push_back(terminal);
	}
	actionColumns.push_back(Grammar::kEndMarker);

	std::vector<Grammar::Symbol> gotoColumns;
	for (const auto &nonTerminal : grammar_.getNonTerminals()) {
		if (nonTerminal != grammar_.getAugmentedStartSymbol()) {
			gotoColumns.push_back(nonTerminal);
		}
	}

	os << "SLR(1) Parsing Table:\n";
	os << std::left << std::setw(8) << "State";
	for (const auto &symbol : actionColumns) {
		os << std::setw(10) << symbol;
	}
	for (const auto &symbol : gotoColumns) {
		os << std::setw(10) << symbol;
	}
	os << '\n';

	for (const auto &state : automaton_.getStates()) {
		os << std::left << std::setw(8) << ("I" + std::to_string(state.id));

		for (const auto &terminal : actionColumns) {
			const auto action = getAction(state.id, terminal);
			os << std::setw(10) << action.toString();
		}

		for (const auto &nonTerminal : gotoColumns) {
			const auto target = getGoto(state.id, nonTerminal);
			os << std::setw(10) << (target.has_value() ? std::to_string(*target) : "");
		}
		os << '\n';
	}
	os << '\n';

	os << "Productions (reduce indices):\n";
	for (const auto &rule : productionRules_) {
		os << "  r" << rule.index << ": " << rule.toString() << '\n';
	}
	os << '\n';
}

void SLRParsingTable::printConflicts(std::ostream &os) const {
	if (conflicts_.empty()) {
		os << "No conflicts found. Grammar is SLR(1).\n\n";
		return;
	}

	os << "Conflicts detected. Grammar is NOT SLR(1):\n";
	for (const auto &conflict : conflicts_) {
		os << "  State I" << conflict.state << ", symbol '" << conflict.symbol << "': "
		   << conflict.kind << " (existing=" << conflict.existing.toString()
		   << ", incoming=" << conflict.incoming.toString() << ")\n";
	}
	os << '\n';
}

ActionEntry SLRParsingTable::getAction(const int stateId, const Grammar::Symbol &terminal) const {
	const auto it = actionTable_.find({stateId, terminal});
	if (it == actionTable_.end()) {
		return {};
	}
	return it->second;
}

std::optional<int> SLRParsingTable::getGoto(const int stateId, const Grammar::Symbol &nonTerminal) const {
	const auto it = gotoTable_.find({stateId, nonTerminal});
	if (it == gotoTable_.end()) {
		return std::nullopt;
	}
	return it->second;
}

std::size_t SLRParsingTable::actionEntryCount() const {
	return actionTable_.size();
}

std::size_t SLRParsingTable::gotoEntryCount() const {
	return gotoTable_.size();
}

std::size_t SLRParsingTable::estimatedTableBytes() const {
	return actionTable_.size() * sizeof(std::pair<const std::pair<int, Grammar::Symbol>, ActionEntry>) +
		gotoTable_.size() * sizeof(std::pair<const std::pair<int, Grammar::Symbol>, int>);
}

void SLRParsingTable::collectProductionRules() {
	int index = 0;
	for (const auto &[lhs, rhsList] : grammar_.getProductions()) {
		for (const auto &rhs : rhsList) {
			const auto normalized = normalizeRhs(rhs);
			const ProductionRule rule{index, lhs, normalized};
			productionRules_.push_back(rule);
			productionIndexByBody_[{lhs, normalized}] = index;
			++index;
		}
	}
}

int SLRParsingTable::findProductionIndex(const Grammar::Symbol &lhs, const Grammar::ProductionBody &rhs) const {
	const auto it = productionIndexByBody_.find({lhs, rhs});
	if (it == productionIndexByBody_.end()) {
		throw std::runtime_error("Production index not found for item: " + lhs);
	}
	return it->second;
}

void SLRParsingTable::setAction(const int stateId, const Grammar::Symbol &terminal, const ActionEntry &action) {
	const auto key = std::make_pair(stateId, terminal);
	const auto it = actionTable_.find(key);
	if (it == actionTable_.end()) {
		actionTable_[key] = action;
		return;
	}

	if (it->second == action) {
		return;
	}

	conflicts_.push_back({stateId, terminal, it->second, action, classifyConflict(it->second, action)});
}

Grammar::ProductionBody SLRParsingTable::normalizeRhs(const Grammar::ProductionBody &rhs) {
	if (rhs.size() == 1 && rhs.front() == Grammar::kEpsilon) {
		return {};
	}
	return rhs;
}

std::string SLRParsingTable::classifyConflict(const ActionEntry &existing, const ActionEntry &incoming) {
	const bool existingShift = existing.type == ActionType::Shift;
	const bool incomingShift = incoming.type == ActionType::Shift;
	const bool existingReduce = existing.type == ActionType::Reduce;
	const bool incomingReduce = incoming.type == ActionType::Reduce;

	if ((existingShift && incomingReduce) || (existingReduce && incomingShift)) {
		return "shift/reduce conflict";
	}
	if (existingReduce && incomingReduce) {
		return "reduce/reduce conflict";
	}
	return "action conflict";
}

