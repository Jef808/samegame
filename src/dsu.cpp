//
// Created by jfa on 20-10-28.
//

#include "dsu.h"
#include <numeric>


DSU::DSU ()
    : parent{0}
{
  reset ();
}


int DSU::find_rep (int ndx)
{
  if (parent[ndx] == ndx)
    {
      return ndx;
    }

  return parent[ndx] = find_rep (parent[ndx]);
}


void DSU::unite (int a, int b)
{
  a = find_rep (a);
  b = find_rep (b);

  if (a != b)
    {
      if (groups[a].size () < groups[b].size ())
        {
          std::swap (a, b);
        }
      while (!groups[b].empty ())
        {
          int v = groups[b].back ();
          groups[b].pop_back ();
          parent[v] = a;
          groups[a].push_back (v);
        }
    }
}


void DSU::reset ()
{
  std::iota (parent.begin (), parent.end (), 0);
  for (int v = 0; v < NB_CELLS; ++v)
    {
      groups[v] = std::vector<int> (1, v);
    }
}