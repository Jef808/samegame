#include <algorithm>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>
#include "dsu.h"


TEST(CanConstructADSU, BasicAssertions)
{
    auto dsu = DSU<uint8_t, 225>();

    ASSERT_TRUE(std::all_of(dsu.begin(), dsu.end(),
                          [](const auto& cluster) {
                              return cluster.members.size() == 1;
                          }));
}

TEST(CanUniteSmallClusters, BasicAssertions)
{
    auto dsu = DSU<uint8_t, 225>();
    dsu.reset();
    dsu.unite(0, 1);
    dsu.unite(1, 2);

    using Cluster = DSU<uint8_t, 225>::Cluster;
    using Index = Cluster::Index;

    Index rep = 1;
    auto members = std::set { 0, 1, 2 };

    Cluster* res = dsu.get_cluster(0);
    std::sort(res->begin(), res->end());

    ASSERT_EQ(res->rep, rep);
    ASSERT_EQ(res->members, members);
}
