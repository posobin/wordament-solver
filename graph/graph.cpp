#include "graph.h"

#include <vector>
#include <string>
#include <utility>

Graph::Graph(std::vector<std::vector<std::string>>& table)
{
  std::vector<std::pair<int, int>> adjacent =
  {{-1, -1}, {-1, 0}, {-1, 1},
   {0, -1}//,{0, 0},  {0, 1},
   /*{1, -1},  {1, 0},  {1, 1}*/};
  std::vector<std::vector<std::vector<int>>> indices(table.size(),
      std::vector<std::vector<int>>(table[0].size()));

  for (int i = 0; i < table.size(); ++i)
    for (int j = 0; j < table[i].size(); ++j)
    {
      bool double_cell = false;
      std::string content(table[i][j]);
      if (content == "") continue;
      if (table[i][j].size() == 3 && table[i][j][1] == '/')
      {
        double_cell = true;
        this->vertices.push_back(Vertex{{}, std::string(&content[2])});
        content = content[0];
      }
      indices[i][j].push_back(vertices.size());
      this->vertices.push_back(Vertex{{}, content});

      for (auto pr : adjacent)
        if (i+pr.first >= 0 && j+pr.second >= 0 &&
            i+pr.first < table.size() && j+pr.second < table[0].size())
          for (auto index : indices[i+pr.first][j+pr.second])
          {
            if (double_cell)
            {
              this->vertices[index].edges.push_back(this->vertices.size()-2);
              this->vertices[this->vertices.size()-2].edges.push_back(index);
            }
            this->vertices[index].edges.push_back(this->vertices.size()-1);
            this->vertices.back().edges.push_back(index);
          }
    }
}
