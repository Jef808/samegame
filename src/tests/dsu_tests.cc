
#include <algorithm>
#include <fstream>
#include <numeric>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>
#include "samegame.h"
#include "dsu.h"

namespace sg {

namespace {

    using sg::Cell;
    using sg::MAX_CELLS;
    using Cluster = sg::State::Cluster;
    using Container = Cluster::Container;
    using DSU = DSU<Cluster, MAX_CELLS>;

    std::string to_string(const Container& cont) {
        std::stringstream ss;
        ss << '{';
        for (auto m : cont) {
            ss << m << ' ';
        }
        ss << '}';
        return ss.str();
    }

    class DsuTest : public ::testing::Test {
    protected:
        const std::string filepath = "../data/input.txt";

        DsuTest()
            : dsu()
        {}

        void SetUp() override
        {
            spdlog::default_logger()->set_level(spdlog::level::debug);
        }

        DSU dsu;
    };



 /**
 * Trivial default constructor
 */
    TEST_F(DsuTest, ClusterDefaultInitializesWithTemplateParameter)
    {
        using ClusterT1 = ClusterT<int, 1>;
        using ContainerT1 = ClusterT1::Container;

        Cluster cluster_sg;
        ClusterT1 cluster_t1;

        const Container expected_members_sg { CELL_NONE };
        const ContainerT1  expected_members_t1 { 1 };

        EXPECT_EQ(cluster_sg.rep, CELL_NONE);
        EXPECT_EQ(cluster_sg.members, expected_members_sg);

        EXPECT_EQ(cluster_t1.rep, 1);
        EXPECT_EQ(cluster_t1.members, expected_members_t1);
    }

    TEST_F(DsuTest, DSUDefaultConstructor)
    {
        std::array<Cell, MAX_CELLS> index;
        std::iota(index.begin(), index.end(), 0);

        DSU& _dsu = dsu;

        auto dsu_has_expected_default_cluster = [&_dsu](const Cell ndx)
        {
            bool res = _dsu.get_cluster(ndx).members == Container { ndx };
            if (!res) {
                spdlog::info("Failure: Cluster at index {} is {}\n but expected members are {}", ndx, _dsu.get_cluster(ndx), to_string(Container { ndx }));
            }
            return res;
        };

        EXPECT_TRUE(std::all_of(index.begin(), index.end(), dsu_has_expected_default_cluster));
    }

    TEST_F(DsuTest, ComparingClustersChecksMembersNotRepresentative)
    {
        auto cluster_1 = Cluster(1, {1, 2, 3, 4, 5});
        auto cluster_4 = Cluster(4, {1, 2, 3 , 4, 5});

        EXPECT_EQ(cluster_1, cluster_4);
    }

    TEST_F(DsuTest, ComparingClustersIgnoreMembersOrder)
    {
        auto cluster_1 = Cluster(1, {1, 2, 3, 4, 5});
        auto cluster_2 = Cluster(1, {2, 4, 1, 5, 3});

        EXPECT_EQ(cluster_1, cluster_2);
    }

    TEST_F(DsuTest, CanUniteSmallClusters)
    {
        dsu.unite(0, 1);
        dsu.unite(1, 2);

        Cluster res_0 = dsu.get_cluster(0);
        Cluster res_1 = dsu.get_cluster(1);
        Cluster res_2 = dsu.get_cluster(2);

        Cluster expected = Cluster(0, Container { 0, 1, 2 });

        EXPECT_EQ(res_0, expected);
        EXPECT_EQ(res_1, expected);
        EXPECT_EQ(res_2, expected);
    }

} // namespace
} // namespace sg
