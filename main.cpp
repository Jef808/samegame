#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include "types.h"
#include "samegame.h"


using namespace std;
using namespace sg;


int main(int argc, char *argv[])
{
    State::init();

    std::ifstream _if;
    _if.open("../data/input.txt", ios::in);

    State state { State(_if) };
    _if.close();

    state.display(std::cout);

    ClusterVec& clusters = state.cluster_list();

    std::cout << clusters.size() << std::endl;

    for (Cluster* c : clusters)
    {
        std::cout <<  "size " << c->size() << " { ";
        auto it = c->begin();
        while (it != c->end())// (auto i : *c)
        {
            std::cout << (int)(*it) << ' ';
            ++it;
        }
        std::cout << " }" << std::endl;
    }

    return 0;
}
