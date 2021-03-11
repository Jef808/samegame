#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include "types.h"
#include "samegame.h"


using namespace std;
using namespace sg;


bool CompBiggestSize(const Cluster* a, const Cluster* b)
{
    return a->size() > b->size();
}

struct CompRarestColor {
    CompRarestColor(const State& state)
        : cc(state.m_colors)
        , g(state.m_cells)
    {
    }
    ColorsCounter cc;
    Grid         g;

    bool operator()(const Cluster* a, const Cluster* b) {
        return cc[g[a->front()]] < cc[g[b->front()]];
    }

};

int main(int argc, char *argv[])
{
    State::init();

    std::ifstream _if;
    _if.open("../data/input.txt", ios::in);
    State state { State(_if) };
    _if.close();

    state.display(std::cout, 0, true);

    ClusterVec& clusters = state.cluster_list();

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

    std::array<StateData, 128> sd;
    int ply = 1;
    int x, y;
    sd[ply].ply = ply;
    sd[ply].key = 0;    //TODO fix

    while (!state.is_terminal())
    {
        std::cout << "Choose a move (column row)" << endl;
        std::cin >> x >> y; std::cin.ignore();

        Cell chosen = x + (14 - y) * WIDTH;

        std::cout << "x=" << x << ", y=" << y;
        std::cout << " Choosing " << (int)chosen << std::endl;

        state.apply_action(chosen, sd[ply]);
        ++ply;

        state.display(std::cout, chosen, true);
    }


    return 0;
}
