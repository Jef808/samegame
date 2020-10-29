//
// Created by jfa on 20-10-29.
//

#ifndef AGENT_TEST_H_
#define AGENT_TEST_H_

#include "Agent.h"


static std::string default_clr = "red";

static const std::vector<int> default_group = std::vector<int> (1, -1);

static const std::string CHOOSE_ACTION = "Enter p x y to play (x, y)";

class AgentTest : public Agent {
 public:
  AgentTest ();

  explicit AgentTest (std::istream &);

  GameState state;

  void play_turn ();

  void render (std::string &clr = default_clr, const std::vector<int> &group = default_group);

  void run ();

 private:
  static std::tuple<char, int> get_user_input (std::istream &, const std::string &);

  std::tuple<char, std::vector<int> &> get_user_legal (std::istream &, const std::string &);

  void get_groups_slow ();

  static std::ostream &red_on (std::ostream &);

  static std::ostream &blue_on (std::ostream &);

  static std::ostream &mag_on (std::ostream &);

  static std::ostream &color_off (std::ostream &);
};

#endif //AGENT_TEST_H_
