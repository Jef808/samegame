//
// Created by jfa on 20-10-29.
//

#ifndef AGENTRANDOM_H_
#define AGENTRANDOM_H_
#include "AgentTest.h"


class AgentRandom : public AgentTest {
 public:
  explicit AgentRandom (std::istream &);

  int choose_action ();

  void run_rand ();
};

#endif //AGENTRANDOM_H_
