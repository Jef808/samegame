//
// Created by jfa on 20-10-29.
//

#include <fstream>
#include <iostream>
#include "AgentRandom.h"


using namespace std;


int main ()
{
  ios_base::sync_with_stdio (false);
  cin.tie (nullptr);
  ifstream file ("data/input.txt", ios::in);

  AgentRandom agent = AgentRandom (file);

  agent.run_rand ();

  return 0;
}
