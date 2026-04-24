//
// Created by ashehalsadik on 4/24/26.
//

#ifndef CC_SLR_LR_1_PARSER_TREE_H
#define CC_SLR_LR_1_PARSER_TREE_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

struct ParseTreeNode {
	std::string symbol;
	std::vector<std::shared_ptr<ParseTreeNode>> children;

	explicit ParseTreeNode(std::string value);
	ParseTreeNode(std::string value, std::vector<std::shared_ptr<ParseTreeNode>> nodeChildren);
};

void printParseTree(const std::shared_ptr<ParseTreeNode> &root, std::ostream &os, int depth = 0);

#endif //CC_SLR_LR_1_PARSER_TREE_H
