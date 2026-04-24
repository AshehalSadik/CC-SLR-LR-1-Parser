//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_SLR_PARSER_H
#define CC_SLR_LR_1_PARSER_SLR_PARSER_H

#include "grammar.h"
#include "parsing_table.h"
#include "tree.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

struct ParseStep {
	int step = 0;
	std::string stack;
	std::string input;
	std::string action;
};

struct ParseResult {
	bool accepted = false;
	std::string errorMessage;
	std::vector<ParseStep> trace;
	std::shared_ptr<ParseTreeNode> parseTreeRoot;
};

class SLRParser {
public:
	SLRParser(const Grammar &grammar, const SLRParsingTable &table);

	ParseResult parse(const std::vector<Grammar::Symbol> &inputTokens) const;

	static std::vector<Grammar::Symbol> splitInput(const std::string &inputText);
	static void printTrace(const ParseResult &result, std::ostream &os);

private:
	const Grammar &grammar_;
	const SLRParsingTable &table_;

	static std::string formatStack(const std::vector<int> &states, const std::vector<Grammar::Symbol> &symbols);
	static std::string formatRemainingInput(const std::vector<Grammar::Symbol> &tokens, std::size_t index);
};

#endif //CC_SLR_LR_1_PARSER_SLR_PARSER_H
