#include "trienode.h"
#include <algorithm>
#include <cctype>

TrieNode::TrieNode() :
  isTerminal(false), descendants(std::vector<TrieNode*>(26, NULL)) { }

static bool deleteAll(TrieNode* element) { delete element; return true; }

TrieNode::~TrieNode()
{
  remove_if(this->descendants.begin(), this->descendants.end(), deleteAll);
}

TrieNode* TrieNode::addLetter(char letter)
{
  if (!isalpha(letter)) return this;
  if (*findLetter(letter) == NULL)
    this->descendants[toupper(letter) - 65] = new TrieNode();
  return *findLetter(letter);
}

TrieNode** TrieNode::findLetter(char letter)
{
  return &this->descendants[toupper(letter) - 65];
}
