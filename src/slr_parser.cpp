//
// Created by ashehalsadik on 4/24/26.
//

#include "slr_parser.h"

#include <algorithm>
#include <sstream>

SLRParser::SLRParser(const Grammar &grammar, const SLRParsingTable &table)
	: grammar_(grammar), table_(table) {
}

ParseResult SLRParser::parse(const std::vector<Grammar::Symbol> &inputTokens) const {
	ParseResult result;

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

		ParseStep traceStep;
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

std::vector<Grammar::Symbol> SLRParser::splitInput(const std::string &inputText) {
	std::vector<Grammar::Symbol> tokens;
	std::stringstream ss(inputText);
	Grammar::Symbol token;
	while (ss >> token) {
		tokens.push_back(token);
	}
	return tokens;
}

void SLRParser::printTrace(const ParseResult &result, std::ostream &os) {
	os << "SLR(1) Parsing Trace:\n";
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

std::string SLRParser::formatStack(const std::vector<int> &states, const std::vector<Grammar::Symbol> &symbols) {
	std::ostringstream oss;
	oss << states.front();
	for (std::size_t i = 0; i < symbols.size(); ++i) {
		oss << ' ' << symbols[i] << ' ' << states[i + 1];
	}
	return oss.str();
}

std::string SLRParser::formatRemainingInput(const std::vector<Grammar::Symbol> &tokens, const std::size_t index) {
	std::ostringstream oss;
	for (std::size_t i = index; i < tokens.size(); ++i) {
		oss << tokens[i];
		if (i + 1 < tokens.size()) {
			oss << ' ';
		}
	}
	return oss.str();
}

