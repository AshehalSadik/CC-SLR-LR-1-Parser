//
// Created by ashehalsadik on 4/24/26.
//

#include "lr1_parser.h"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <utility>

bool LR1Item::operator<(const LR1Item &other) const {
	if (lhs != other.lhs) {
		return lhs < other.lhs;
	}
	if (rhs != other.rhs) {
		return rhs < other.rhs;
	}
	if (dot != other.dot) {
		return dot < other.dot;
	}
	return lookahead < other.lookahead;
}

bool LR1Item::operator==(const LR1Item &other) const {
	return lhs == other.lhs && rhs == other.rhs && dot == other.dot && lookahead == other.lookahead;
}

LR1Automaton::LR1Automaton(const Grammar &grammar) : grammar_(grammar) {
}

void LR1Automaton::build() {
	states_.clear();
	transitions_.clear();

	const auto &productions = grammar_.getProductions();
	const auto &augmentedStart = grammar_.getAugmentedStartSymbol();
	if (augmentedStart.empty()) {
		throw std::runtime_error("Grammar must be augmented before building LR(1) items.");
	}

	const auto startIt = productions.find(augmentedStart);
	if (startIt == productions.end() || startIt->second.empty()) {
		throw std::runtime_error("Augmented start production not found.");
	}

	const auto firstSets = grammar_.computeFirstSets();

	const LR1Item startItem{augmentedStart, normalizeRhs(startIt->second.front()), 0, Grammar::kEndMarker};
	LR1State initial;
	initial.id = 0;
	initial.items = closure({startItem}, firstSets);
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
			const auto targetItems = goTo(states_[static_cast<std::size_t>(currentStateId)].items, symbol, firstSets);
			if (targetItems.empty()) {
				continue;
			}

			int existing = findExistingState(targetItems);
			if (existing < 0) {
				LR1State newState;
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

std::optional<int> LR1Automaton::getTransition(const int stateId, const Grammar::Symbol &symbol) const {
	const auto it = transitions_.find({stateId, symbol});
	if (it == transitions_.end()) {
		return std::nullopt;
	}
	return it->second;
}

void LR1Automaton::print(std::ostream &os) const {
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

std::set<LR1Item> LR1Automaton::closure(
	const std::set<LR1Item> &seed,
	const std::map<Grammar::Symbol, std::set<Grammar::Symbol>> &firstSets) const {
	std::set<LR1Item> result = seed;
	bool changed = true;

	while (changed) {
		changed = false;
		std::vector<LR1Item> toAdd;

		for (const auto &item : result) {
			if (item.dot >= item.rhs.size()) {
				continue;
			}

			const auto &symbolAfterDot = item.rhs[item.dot];
			if (grammar_.getNonTerminals().count(symbolAfterDot) == 0U) {
				continue;
			}

			Grammar::ProductionBody betaA;
			if (item.dot + 1 < item.rhs.size()) {
				betaA.assign(
					std::next(item.rhs.begin(), static_cast<Grammar::ProductionBody::difference_type>(item.dot + 1)),
					item.rhs.end());
			}
			betaA.push_back(item.lookahead);

			const auto lookaheads = grammar_.firstOfSequence(betaA, firstSets);
			const auto prodIt = grammar_.getProductions().find(symbolAfterDot);
			if (prodIt == grammar_.getProductions().end()) {
				throw std::runtime_error("Missing productions for non-terminal: " + symbolAfterDot);
			}

			for (const auto &rhs : prodIt->second) {
				const auto normalized = normalizeRhs(rhs);
				for (const auto &lookahead : lookaheads) {
					if (lookahead == Grammar::kEpsilon) {
						continue;
					}
					toAdd.push_back({symbolAfterDot, normalized, 0, lookahead});
				}
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

std::set<LR1Item> LR1Automaton::goTo(
	const std::set<LR1Item> &stateItems,
	const Grammar::Symbol &symbol,
	const std::map<Grammar::Symbol, std::set<Grammar::Symbol>> &firstSets) const {
	std::set<LR1Item> moved;
	for (const auto &item : stateItems) {
		if (item.dot < item.rhs.size() && item.rhs[item.dot] == symbol) {
			LR1Item advanced = item;
			++advanced.dot;
			moved.insert(advanced);
		}
	}

	if (moved.empty()) {
		return {};
	}
	return closure(moved, firstSets);
}

int LR1Automaton::findExistingState(const std::set<LR1Item> &items) const {
	for (const auto &state : states_) {
		if (state.items == items) {
			return state.id;
		}
	}
	return -1;
}

Grammar::ProductionBody LR1Automaton::normalizeRhs(const Grammar::ProductionBody &rhs) {
	if (rhs.size() == 1 && rhs.front() == Grammar::kEpsilon) {
		return {};
	}
	return rhs;
}

std::string LR1Automaton::itemToString(const LR1Item &item) {
	std::ostringstream oss;
	oss << '[' << item.lhs << " -> ";

	if (item.rhs.empty()) {
		if (item.dot == 0) {
			oss << ". ";
		}
		oss << Grammar::kEpsilon;
		if (item.dot == 1) {
			oss << " .";
		}
		oss << ", " << item.lookahead << ']';
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

	oss << ", " << item.lookahead << ']';
	return oss.str();
}

LR1ParsingTable::LR1ParsingTable(const Grammar &grammar, const LR1Automaton &automaton)
	: grammar_(grammar), automaton_(automaton) {
}

void LR1ParsingTable::build() {
	actionTable_.clear();
	gotoTable_.clear();
	conflicts_.clear();
	productionRules_.clear();
	productionIndexByBody_.clear();

	collectProductionRules();

	for (const auto &state : automaton_.getStates()) {
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
				item.rhs.front() == grammar_.getStartSymbol() &&
				item.lookahead == Grammar::kEndMarker;

			if (isAugmentedAcceptItem) {
				setAction(state.id, Grammar::kEndMarker, {ActionType::Accept, -1, -1});
				continue;
			}

			const int productionIndex = findProductionIndex(item.lhs, item.rhs);
			setAction(state.id, item.lookahead, {ActionType::Reduce, -1, productionIndex});
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

void LR1ParsingTable::print(std::ostream &os) const {
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

	os << "LR(1) Parsing Table:\n";
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

void LR1ParsingTable::printConflicts(std::ostream &os) const {
	if (conflicts_.empty()) {
		os << "No conflicts found. Grammar is LR(1).\n\n";
		return;
	}

	os << "Conflicts detected. Grammar is NOT LR(1):\n";
	for (const auto &conflict : conflicts_) {
		os << "  State I" << conflict.state << ", symbol '" << conflict.symbol << "': "
		   << conflict.kind << " (existing=" << conflict.existing.toString()
		   << ", incoming=" << conflict.incoming.toString() << ")\n";
	}
	os << '\n';
}

ActionEntry LR1ParsingTable::getAction(const int stateId, const Grammar::Symbol &terminal) const {
	const auto it = actionTable_.find({stateId, terminal});
	if (it == actionTable_.end()) {
		return {};
	}
	return it->second;
}

std::optional<int> LR1ParsingTable::getGoto(const int stateId, const Grammar::Symbol &nonTerminal) const {
	const auto it = gotoTable_.find({stateId, nonTerminal});
	if (it == gotoTable_.end()) {
		return std::nullopt;
	}
	return it->second;
}

std::size_t LR1ParsingTable::actionEntryCount() const {
	return actionTable_.size();
}

std::size_t LR1ParsingTable::gotoEntryCount() const {
	return gotoTable_.size();
}

std::size_t LR1ParsingTable::estimatedTableBytes() const {
	return actionTable_.size() * sizeof(std::pair<const std::pair<int, Grammar::Symbol>, ActionEntry>) +
		   gotoTable_.size() * sizeof(std::pair<const std::pair<int, Grammar::Symbol>, int>);
}

void LR1ParsingTable::collectProductionRules() {
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

int LR1ParsingTable::findProductionIndex(const Grammar::Symbol &lhs, const Grammar::ProductionBody &rhs) const {
	const auto it = productionIndexByBody_.find({lhs, rhs});
	if (it == productionIndexByBody_.end()) {
		throw std::runtime_error("Production index not found for item: " + lhs);
	}
	return it->second;
}

void LR1ParsingTable::setAction(const int stateId, const Grammar::Symbol &terminal, const ActionEntry &action) {
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

Grammar::ProductionBody LR1ParsingTable::normalizeRhs(const Grammar::ProductionBody &rhs) {
	if (rhs.size() == 1 && rhs.front() == Grammar::kEpsilon) {
		return {};
	}
	return rhs;
}

std::string LR1ParsingTable::classifyConflict(const ActionEntry &existing, const ActionEntry &incoming) {
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

LR1Parser::LR1Parser(const Grammar &grammar, const LR1ParsingTable &table)
	: grammar_(grammar), table_(table) {
}

LR1ParseResult LR1Parser::parse(const std::vector<Grammar::Symbol> &inputTokens) const {
	LR1ParseResult result;

	std::vector<Grammar::Symbol> tokens = inputTokens;
	if (tokens.empty() || tokens.back() != Grammar::kEndMarker) {
		tokens.push_back(Grammar::kEndMarker);
	}

	std::vector<int> stateStack{0};
	std::vector<Grammar::Symbol> symbolStack;
	std::vector<std::shared_ptr<ParseTreeNode>> nodeStack;

	std::size_t inputIndex = 0;
	int step = 1;

	while (true) {
		const int currentState = stateStack.back();
		const auto &lookahead = tokens[inputIndex];
		const ActionEntry action = table_.getAction(currentState, lookahead);

		LR1ParseStep traceStep;
		traceStep.step = step++;
		traceStep.stack = formatStack(stateStack, symbolStack);
		traceStep.input = formatRemainingInput(tokens, inputIndex);

		if (action.type == ActionType::Shift) {
			traceStep.action = "Shift " + std::to_string(action.targetState);
			result.trace.push_back(traceStep);

			symbolStack.push_back(lookahead);
			stateStack.push_back(action.targetState);
			nodeStack.push_back(std::make_shared<ParseTreeNode>(lookahead));
			++inputIndex;
			continue;
		}

		if (action.type == ActionType::Reduce) {
			const auto &rule = table_.getProductionRules()[static_cast<std::size_t>(action.productionIndex)];
			traceStep.action = "Reduce " + rule.toString();
			result.trace.push_back(traceStep);

			const std::size_t rhsLength = rule.rhs.size();
			if (symbolStack.size() < rhsLength || stateStack.size() < rhsLength + 1) {
				result.errorMessage = "Stack underflow while reducing " + rule.toString();
				return result;
			}

			std::vector<std::shared_ptr<ParseTreeNode>> children;
			for (std::size_t i = 0; i < rhsLength; ++i) {
				stateStack.pop_back();
				symbolStack.pop_back();
				children.push_back(nodeStack.back());
				nodeStack.pop_back();
			}
			std::reverse(children.begin(), children.end());

			if (rhsLength == 0) {
				children.push_back(std::make_shared<ParseTreeNode>(Grammar::kEpsilon));
			}

			const auto gotoState = table_.getGoto(stateStack.back(), rule.lhs);
			if (!gotoState.has_value()) {
				result.errorMessage = "Missing GOTO transition for state " +
									  std::to_string(stateStack.back()) + " and symbol " + rule.lhs;
				return result;
			}

			symbolStack.push_back(rule.lhs);
			stateStack.push_back(*gotoState);
			nodeStack.push_back(std::make_shared<ParseTreeNode>(rule.lhs, std::move(children)));
			continue;
		}

		if (action.type == ActionType::Accept) {
			traceStep.action = "Accept";
			result.trace.push_back(traceStep);

			result.accepted = true;
			if (!nodeStack.empty()) {
				result.parseTreeRoot = nodeStack.back();
			}
			return result;
		}

		traceStep.action = "Error";
		result.trace.push_back(traceStep);
		result.errorMessage = "No ACTION entry for state " + std::to_string(currentState) +
							  " with lookahead '" + lookahead + "'";
		return result;
	}
}

std::vector<Grammar::Symbol> LR1Parser::splitInput(const std::string &inputText) {
	std::vector<Grammar::Symbol> tokens;
	std::stringstream ss(inputText);
	Grammar::Symbol token;
	while (ss >> token) {
		tokens.push_back(token);
	}
	return tokens;
}

void LR1Parser::printTrace(const LR1ParseResult &result, std::ostream &os) {
	os << "LR(1) Parsing Trace:\n";
	os << "Step | Stack | Input | Action\n";

	for (const auto &step : result.trace) {
		os << step.step << " | " << step.stack << " | " << step.input << " | " << step.action << '\n';
	}

	if (result.accepted) {
		os << "\nResult: String accepted successfully.\n\n";
	} else {
		os << "\nResult: Rejected. " << result.errorMessage << "\n\n";
	}
}

std::string LR1Parser::formatStack(const std::vector<int> &states, const std::vector<Grammar::Symbol> &symbols) {
	std::ostringstream oss;
	oss << states.front();
	for (std::size_t i = 0; i < symbols.size(); ++i) {
		oss << ' ' << symbols[i] << ' ' << states[i + 1];
	}
	return oss.str();
}

std::string LR1Parser::formatRemainingInput(const std::vector<Grammar::Symbol> &tokens, const std::size_t index) {
	std::ostringstream oss;
	for (std::size_t i = index; i < tokens.size(); ++i) {
		oss << tokens[i];
		if (i + 1 < tokens.size()) {
			oss << ' ';
		}
	}
	return oss.str();
}

