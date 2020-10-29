//
// Created by jfa on 20-10-28.
//

#include <fstream>
#include <iostream>
#include "AgentTest.h"


using namespace std;


int main ()
{
  ios_base::sync_with_stdio (false);
  cin.tie (nullptr);
  ifstream file ("data/input.txt", ios::in);

  AgentTest agent = AgentTest (file);

  agent.run ();

  return 0;
}
