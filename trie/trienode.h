#ifndef __TRIENODE_H_INCLUDED__
#define __TRIENODE_H_INCLUDED__

#include <vector>
#include <utility>

class TrieNode {
private:
  std::vector<TrieNode*> descendants;
public:
  bool isTerminal;
  TrieNode();
  ~TrieNode();
  TrieNode* addLetter(char letter);
  TrieNode** findLetter(char letter);
};

#endif
