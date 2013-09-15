#include "trie.h"
#include "trienode.h"
#include <vector>
#include <string>

Trie::Trie()
{
  root = new TrieNode();
}

Trie::~Trie()
{
  delete root;
}

TrieNode* Trie::getNode(const std::string& str)
{
  TrieNode* node = this->root;
  for (int i = 0; i < str.size() && node != NULL; ++i)
    node = *node->findLetter(str[i]);
  return node;
}

void Trie::add(const std::string& str)
{
  TrieNode* node = this->root;
  for (auto ch : str) node = node->addLetter(ch);
  node->isTerminal = true;
}

bool Trie::contains(const std::string& str)
{
  TrieNode* node = this->getNode(str);
  return (node != NULL && node->isTerminal);
}
