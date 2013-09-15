#ifndef __TRIE_H_INCLUDED__
#define __TRIE_H_INCLUDED__

#include "trienode.h"
#include <string>

class Trie {
private:
  TrieNode* root;
public:
  Trie();
  ~Trie();
  TrieNode* getNode(const std::string& str);
  void add(const std::string& str);
  bool contains(const std::string& str);
};

#endif
