//
// Created by jfa on 20-10-29.
//
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include "AgentTest.h"


AgentTest::AgentTest ()
    : state ()
{

}


AgentTest::AgentTest (std::istream &_in)
    : state (_in)
{
  set_legal_groups (state);
}


constexpr char esc_char = 27;


std::ostream &AgentTest::red_on (std::ostream &os)
{
  return os << esc_char << "[31m";
}


std::ostream &AgentTest::blue_on (std::ostream &os)
{
  return os << esc_char << "[34m";
}


std::ostream &AgentTest::mag_on (std::ostream &os)
{
  return os << esc_char << "[35m";
}


std::ostream &AgentTest::color_off (std::ostream &os)
{
  return os << esc_char << "[0m";
}


std::tuple<char, int> AgentTest::get_user_input (std::istream &_in, const std::string &prompt)
{
  while (true)
    {
      char command{0};
      int action_x{-1}, action_y{-1};
      _in >> command >> action_x >> action_y;

      if (action_y == -1)
        {
          std::cout << "invalid entry";
          continue;
        }

      if (command != 'p' && command != 'g' && command != 's')
        {
          std::cout << command << " is an invalid command." << std::endl;
          continue;
        }

      int action = action_x + (HEIGHT - 1 - action_y) * HEIGHT;

      if (action < 0 || action > 224)
        {
          std::cout << "(x, y) = (" << action_x << ' ' << action_y << ") is out of bounds." << std::endl;
          continue;
        }

      return {command, action};
    }
}


std::tuple<char, std::vector<int> &> AgentTest::get_user_legal (std::istream &_in, const std::string &prompt)
{
  while (true)
    {
      char command{0};
      int action{0};

      std::tie (command, action) = get_user_input (std::cin, prompt);

      std::vector<int> &group = dsu.groups[dsu.find_rep (action)];

      if (command == 'g')
        {
          return {command, group};
        }
      if (command == 's')
        {
          return {command, group};
        }

      if (group.size () < 2)
        {
          render (default_clr, group);
          std::cout << "Invalid move" << std::endl;
          continue;
        }

      return {command, group};
    }
}


void AgentTest::play_turn ()
{
  char command{0};
  std::vector<int> &group = dsu.groups.front ();

  std::cout << CHOOSE_ACTION << std::endl;

  std::tie (command, group) = get_user_legal (std::cin, CHOOSE_ACTION);

  if (command == 'p')
    {
      render (default_clr, group);
      int points = score_move (group.size ());
      std::cout << "+ " << points << " points!\n\n" << std::endl;
      sleep (1);

      state.step (group);
      set_legal_groups (state);
      render ();
      score += points;
      std::cout << "\nScore is now " << score << '\n' << std::endl;
    }

  else if (command == 'g')
    {
      std::cout << "group is { ";
      for (int a: group)
        {
          std::cout << '(' << a % WIDTH << ' ' << 14 - a / HEIGHT << "), ";
        }
      std::cout << "}" << std::endl;

      render (default_clr, group);
    }

  else if (command == 's')
    {
      get_groups_slow ();
    }
}


void AgentTest::render (std::string &clr, const std::vector<int> &group)
{
  for (int y = 0; y < HEIGHT; ++y)
    {
      std::cout << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";

      for (int x = 0; x < WIDTH; ++x)
        {
          if (std::find (group.begin (), group.end (), x + y * WIDTH) != group.end ())
            {
              if (clr == "red")
                {
                  red_on (std::cout) << state[x + y * WIDTH];
                }
              else
                {
                  blue_on (std::cout) << state[x + y * WIDTH];
                }
              color_off (std::cout) << ' ';
            }
          else if (state[x + y * WIDTH] == 0)
            {
              mag_on (std::cout) << state[x + y * WIDTH];
              color_off (std::cout) << ' ';
            }
          else
            {
              std::cout << state[x + y * WIDTH] << ' ';
            }
        }

      std::cout << std::endl;
    }

  std::cout << std::string (34, '_') << std::endl;

  int x_labels[15]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
  std::cout << std::string (5, ' ');
  for (int x: x_labels)
    {
      std::cout << x << ((x < 10) ? " " : "");
    }

  std::cout << std::endl;
}


void AgentTest::run ()
{
  render ();

  while (!state.is_terminal ())
    {
      play_turn ();
    }

  if (state.is_cleared ())
    {
      score += 1000;
    }

  std::cout << "Game over! Final score: " << score << std::endl;
}


void AgentTest::get_groups_slow ()
{
  dsu_reset ();
  std::vector<int> bt;  // blue text
  std::string blue = "blue";

  /**< Traverse first row */
  for (int i = 0; i < WIDTH - 1; ++i)
    {
      bt.push_back (i);
      bt.push_back (i + 1);
      render (blue, bt);
      sleep (1);
      bt.clear ();
      if (state[i + 1] == 0 || state[i + 1] != state[i])
        {
          continue;
        }
      dsu.unite (i + 1, i);
      render (default_clr, dsu.groups[dsu.find_rep (i + 1)]);
      sleep (1);
    }

  /**< Traverse other rows */
  for (int i = WIDTH; i < NB_CELLS; ++i)
    {
      /**< Look at cell above */
      bt.push_back (i);
      bt.push_back (i - WIDTH);
      render (blue, bt);
      sleep (1);
      bt.clear ();
      if (state[i - WIDTH] != 0 && state[i - WIDTH] == state[i])
        {
          dsu.unite (i - WIDTH, i);
          render (default_clr, dsu.groups[dsu.find_rep (i - WIDTH)]);
          sleep (1);
        }

      /**< Look at cell to the right */
      bt.push_back (i);
      bt.push_back (i + 1);
      render (blue, bt);
      sleep (1);
      bt.clear ();
      if ((i + 1) % WIDTH == 0 || state[i + 1] == 0 || state[i + 1] != state[i])
        {
          continue;
        }
      dsu.unite (i + 1, i);
      render (default_clr, dsu.groups[dsu.find_rep (i + 1)]);
      sleep (1);
    }
}
