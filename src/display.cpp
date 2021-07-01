#include "display.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <string>
#include <sstream>
#include <thread>

#include "dsu.h"
#include "clusterhelper.h"

namespace sg::display {


enum class Color_codes : int
{
  BLACK = 30,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  WHITE = 37,
  B_BLACK = 90,
  B_RED = 91,
  B_GREEN = 92,
  B_YELLOW = 93,
  B_BLUE = 94,
  B_MAGENTA = 95,
  B_CYAN = 96,
  B_WHITE = 97,
};

enum class Shape : int
{
  SQUARE,
  DIAMOND,
  B_DIAMOND,
  NONE
};

std::map<Shape, std::string> Shape_codes{
    {Shape::SQUARE, "\u25A0"},
    {Shape::DIAMOND, "\u25C6"},
    {Shape::B_DIAMOND, "\u25C7"},
};

std::string shape_unicode(Shape s) { return Shape_codes[s]; }

std::string color_unicode(Color c)
{
  return std::to_string(to_integral(Color_codes(to_integral(c) + 90)));
}

std::string print_cell(
    const sg::Grid& grid,
    Cell ndx,
    Output output_mode,
    const ClusterT<Cell, CELL_NONE>& cluster = ClusterT<Cell, CELL_NONE>())
{
  bool ndx_in_cluster =
      find(cluster.cbegin(), cluster.cend(), ndx) != cluster.cend();
  std::stringstream ss;

  if (output_mode == Output::CONSOLE)
  {
    Shape shape = ndx == cluster.rep ? Shape::B_DIAMOND
                  : ndx_in_cluster   ? Shape::DIAMOND
                                     : Shape::SQUARE;

    ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape)
       << "\033[0m";

    return ss.str();
  }
  else
  {
    // Underline the cluster representative and output the cluster in bold
    std::string formatter = "";
    if (ndx_in_cluster)
      formatter += "\e[1m]";
    if (ndx == cluster.rep)
      formatter += "\033[4m]";

    ss << formatter << std::to_string(to_integral(grid[ndx])) << "\033[0m\e[0m";

    return ss.str();
  }
}

const int x_labels[15]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

const std::string
to_string(const Grid& grid, const Cell cell, sg::Output output_mode)
{
  using Cluster = ClusterT<Cell, MAX_CELLS>;

  Cluster cluster = clusters::get_cluster(grid, cell);
  cluster.rep = cell;
  bool labels = true;
  std::stringstream ss{std::string("\n")};

  // For every row
  for (int y = 0; y < HEIGHT; ++y)
  {
    if (labels)
    {
      ss << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
    }
    // Print row except last entry
    for (int x = 0; x < WIDTH - 1; ++x)
    {
      ss << print_cell(grid, x + y * WIDTH, output_mode, cluster) << ' ';
    }
    // Print last entry,
    ss << print_cell(grid, WIDTH - 1 + y * WIDTH, output_mode, cluster) << '\n';
  }

  if (labels)
  {
    ss << std::string(34, '_') << '\n' << std::string(5, ' ');

    for (int x : x_labels)
    {

      ss << x << ((x < 10) ? " " : "");
    }
    ss << '\n';
  }

  return ss.str();
}

void enumerate_clusters(std::ostream& _out, const Grid& _grid)
{
  using namespace std::literals::chrono_literals;

  ClusterDataVec cd_vec = clusters::get_valid_clusters_descriptors(_grid);

  for (const auto& cd : cd_vec)
  {
    Cluster cluster = clusters::get_cluster(_grid, cd.rep);
    _out << cluster;
    std::this_thread::sleep_for(500.0ms);
  }
}

void view_clusters(std::ostream& _out, const Grid& _grid)
{
  using namespace std::literals::chrono_literals;

  ClusterDataVec cd_vec = clusters::get_valid_clusters_descriptors(_grid);

  for (const auto& cd : cd_vec)
  {
    _out << to_string(_grid, cd.rep);
    std::this_thread::sleep_for(300.0ms);
  }
}

void view_action_sequence(std::ostream& out,
                          Grid& grid,
                          const std::vector<ClusterData>& actions,
                          int delay_in_ms)
{
  double score = 0;
  ClusterData cd_check{};

  PRINT("Printing ",
        actions.size(),
        " actions from the following starting grid\n",
        to_string(grid));

  for (auto it = actions.begin(); it != actions.end(); ++it)
  {
      PRINT(to_string(grid, it->rep));

      cd_check = clusters::apply_action(grid, it->rep);

      if (cd_check.rep == CELL_NONE || cd_check.color == Color::Empty || cd_check.size < 2)
      {
          PRINT("\n    WARNING: Action number ",
                std::distance(actions.begin(), it),
                " is invalid.");
          if (it != actions.end())
          {
              PRINT(" Skipping the remaining ",
                    std::distance(it, actions.end()),
                    " actions.");
              return;
          }
      }

      int val = it->size - 2;
      val = (val + std::abs(val)) / 2;
      score += std::pow(val, 2);

      PRINT("SCORE : ", score);

      std::this_thread::sleep_for(std::chrono::milliseconds(delay_in_ms));
  }

  score += 1000 * (grid[CELL_BOTTOM_LEFT] == Color::Empty);

  PRINT("FINAL SCORE : ", score);
}

} // namespace sg::display
