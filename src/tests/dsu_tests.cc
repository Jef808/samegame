#include <algorithm>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>
#include "samegame.h"
#include "dsu.h"

using sg::Cell;
using sg::MAX_CELLS;
using sg::Cluster;

namespace {

/**
 * Trivial default constructor
 */
TEST( ClusterHandling, TrivialConstructor )
{
    using Container = Cluster::Container;
    const Cluster cluster {};
    const Container expected_cont { sg::CELL_NONE };

    EXPECT_EQ(cluster.members, expected_cont);
    EXPECT_EQ(cluster.rep, sg::CELL_NONE);
}

TEST( ClusterHandling, DSUTrivialConstructor )
{
    auto dsu = DSU<Cluster, MAX_CELLS>();//dsu { };
    EXPECT_TRUE(std::all_of(dsu.begin(), dsu.end(), [](const auto& m) { return m.size() == 1; }));
    auto cluster_5 = *(dsu.begin() + 5);
    EXPECT_EQ(cluster_5, Cluster(5));
}

TEST(CanUniteSmallClusters, BasicAssertions)
{
    DSU<Cluster, MAX_CELLS> dsu { };
    //dsu.reset();
    dsu.unite(0, 1);
    dsu.unite(1, 2);

    // using Container = Cluster::Container;
    // using Index = Cluster::Index;

    Cluster res = dsu.get_cluster(0);
    // Container members_res = res.members;
    // Index rep_res = res.rep;
    //std::sort(members_res.begin(), members_res.end());

    auto expected = Cluster(1, { 0, 1, 2 } );
    // Cell rep_expected = 1;
    // Container members_expected = { 0, 1, 2 };

    EXPECT_EQ(res, expected);
    //EXPECT_EQ(members_res, members_expected);
}

} // namespace
