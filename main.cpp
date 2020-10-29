#include <fstream>
#include <iostream>
#include "Agent.h"


using namespace std;


int main ()
{
  cout << "wtf!" << endl;
  ios_base::sync_with_stdio (false);
  cin.tie (nullptr);

  ifstream file ("input.txt", ios::in);

  Agent agent = Agent ();
  GameState state (file);

  agent.set_legal_groups (state);

  cout << "Done!" << endl;

  return 0;
}
