#include <algorithm>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>
#include "dsu.h"
#include "samegame.h"


using sg::Cell;
using sg::MAX_CELLS;
using sg::Cluster;

/**
 * Trivial default constructor
 */
TEST( ClusterHandling, TrivialConstructor )
{
    using Container = Cluster::Container;

    const Cluster cluster { };
    EXPECT_EQ(cluster.members, Container { sg::CELL_NONE });
    EXPECT_EQ(cluster.rep, sg::CELL_NONE);
}

TEST( ClusterHandling, DSUTrivialConstructor )
{
    DSU<Cluster, MAX_CELLS> dsu { };
    EXPECT_TRUE(std::all_of(dsu.begin(), dsu.end(), [](const auto& m) { return m.size() == 1; }));
    auto cluster_5 = *(dsu.begin() + 5)
    EXPECT_EQ(cluster_5, Cluster(5));
}

TEST(CanUniteSmallClusters, BasicAssertions)
{
    DSU<Cluster, MAX_CELLS> dsu { };
    //dsu.reset();
    dsu.unite(0, 1);
    dsu.unite(1, 2);

    using Cluster = DSU<Cell, Cell::NB>::Cluster;
    using Container = Cluster::Container;
    using Index = Cluster::Index;

    auto res = dsu.get_cluster(0);
    Container members_res = res.members;
    Index rep_res = res.rep;
    std::sort(members_res.begin(), members_res.end());

    Cell rep_expected = 1;
    Container members_expected = { 0, 1, 2 };

    EXPECT_EQ(rep_res, rep_expected);
    EXPECT_EQ(members_res, members_expected);
}
