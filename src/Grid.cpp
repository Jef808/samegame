#include <vector>
#include <iostream>
#include "constants.h"
#include "Grid.h"
#include <sstream>


Grid::Grid ()
    : cells{0}, clr_counter{0}
{
  //ctor
}


Grid::Grid (std::istream &_in)
    : cells{0}, clr_counter{0}
{
  std::string row_buffer;
  int clr_buffer{0};

  for (int y = 0; y < HEIGHT; ++y)
    {
      getline (_in, row_buffer);
      std::istringstream ss (row_buffer);

      for (int x = 0; x < WIDTH; ++x)
        {
          ss >> clr_buffer;
          cells[x + WIDTH * y] = ++clr_buffer;
          ++clr_counter[clr_buffer];
        }
    }
}


Grid::Grid (const Grid &other) = default;


Grid::Grid (Grid &&other) noexcept
    : cells (other.cells), clr_counter (other.clr_counter)
{
  //ctor
}


Grid::~Grid () = default;


void Grid::clear_group (const std::vector<int> &group)
{
  for (int ndx: group)
    {
      cells[ndx] = 0;
    }
}


void Grid::do_gravity ()
{
  // For all columns
  for (int i = 0; i < WIDTH; ++i)
    {
      int bottom = i + WIDTH * (HEIGHT - 1);
      // look for an empty cell
      while (bottom > WIDTH - 1 && cells[bottom] != 0)
        {
          bottom -= WIDTH;
        }
      // make non-empty cells above drop
      int top = bottom - WIDTH;
      while (top > -1)
        {
          // if top is non-empty, swap its color with empty color and move both pointers up
          if (cells[top] != 0)
            {
              std::swap (cells[top], cells[bottom]);
              bottom -= WIDTH;
            }
          // otherwise move top pointer up
          top -= WIDTH;
        }
    }
}


void Grid::pull_columns ()
{
  int left_col = WIDTH * (HEIGHT - 1);
  // look for an empty column
  while (left_col < NB_CELLS && cells[left_col] != 0)
    {
      ++left_col;
    }
  // pull columns on the right to the empty column
  int right_col = left_col + 1;
  while (right_col < NB_CELLS)
    {
      // if the column is non-empty, swap all its colors with the empty column and move both pointers right
      if (cells[right_col] != 0)
        {
          for (int i = 0; i < HEIGHT; ++i)
            {
              std::swap (cells[right_col - i * WIDTH], cells[left_col - i * WIDTH]);
            }
          ++left_col;
        }
      // otherwise move right pointer right
      ++right_col;
    }
}


void Grid::render (bool labels) const
{
  for (int y = 0; y < HEIGHT; ++y)
    {

      if (labels)
        {
          std::cout << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

      for (int x = 0; x < WIDTH - 1; ++x)
        {
          std::cout << (int) cells[x + y * WIDTH] << ' ';
        }
      std::cout << (int) cells[WIDTH - 1 + y * WIDTH] << std::endl;
    }

  if (labels)
    {
      std::cout << std::string (34, '_') << std::endl;

      int x_labels[15]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
      std::cout << std::string (5, ' ');
      for (int x: x_labels)
        {
          std::cout << x << ((x < 10) ? " " : "");
        }

      std::cout << std::endl;
    }
}
