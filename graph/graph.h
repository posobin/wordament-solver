#ifndef __GRAPH_H_INCLUDED__
#define __GRAPH_H_INCLUDED__

#include <vector>
#include <string>

class Graph
{
private:
  class Vertex
  {
    enum Type 
    {
      NORMAL, OR, BEGIN, END
    };
  public:
    std::vector<int> edges;
    std::string content;
  };
public:
  std::vector<Vertex> vertices;
  Graph(std::vector<std::vector<std::string>>& table);
};

#endif
