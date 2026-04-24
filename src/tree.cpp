//
// Created by ashehalsadik on 4/24/26.
//

#include "tree.h"

#include <iostream>
#include <utility>

ParseTreeNode::ParseTreeNode(std::string value) : symbol(std::move(value)) {
}

ParseTreeNode::ParseTreeNode(std::string value, std::vector<std::shared_ptr<ParseTreeNode>> nodeChildren)
	: symbol(std::move(value)), children(std::move(nodeChildren)) {
}

void printParseTree(const std::shared_ptr<ParseTreeNode> &root, std::ostream &os, const int depth) {
	if (!root) {
		return;
	}

	for (int i = 0; i < depth; ++i) {
		os << "  ";
	}
	os << root->symbol << '\n';

	for (const auto &child : root->children) {
		printParseTree(child, os, depth + 1);
	}
}

