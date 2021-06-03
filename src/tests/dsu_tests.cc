
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <functional>
#include <fstream>
#include <numeric>
#include <string>
#include <sstream>
#include <vector>
//#include "samegame.h"
#include "dsu.h"


namespace {

    using Index = int;
    const Index IndexMax = 10;
    const Index IndexNone = 11;
    using Cluster = ClusterT<int, IndexNone>;
    using Container = Cluster::Container;
    using DSU = DSU<Cluster, IndexMax>;

    // std::string to_string(const Container& cont) {
    //     std::stringstream ss;
    //     ss << '{';
    //     for (auto m : cont) {
    //         ss << m << ' ';
    //     }
    //     ss << '}';
    //     return ss.str();
    // }

    class DsuTest : public ::testing::Test {
    protected:
        DsuTest()
            : dsu()
        {}

        void SetUp() override
        {
            spdlog::default_logger()->set_level(spdlog::level::debug);
        }

        DSU dsu;

        testing::AssertionResult HasDefaultClusters()
        {
            bool result = std::all_of(dsu.begin(), dsu.end(), [n=-1](auto const& cluster) mutable {
                return cluster.members == Container(1, ++n); 
            });
            if (result) {
                return testing::AssertionSuccess();
            }
            return testing::AssertionFailure();
        }
    };


 /**
 * Trivial default constructor
 */
    TEST_F(DsuTest, ClusterDefaultInitializesWithTemplateParameter)
    {
        Cluster cluster {};
        Index rep_expected = IndexNone;
        Container members_expected = Container(1, IndexNone);

        EXPECT_THAT(cluster.rep, rep_expected);
        EXPECT_THAT(cluster.members, testing::ContainerEq(members_expected));

        const Index OtherIndexNone = 20;
        using OtherCluster = ClusterT<int, OtherIndexNone>;
        using OtherContainer = OtherCluster::Container;

        OtherCluster other_cluster {};
        Index other_rep_expected = OtherIndexNone;
        OtherContainer other_members_expected = Container(1, OtherIndexNone);

        EXPECT_THAT(other_cluster.rep, other_rep_expected);
        EXPECT_THAT(other_cluster.members, testing::ContainerEq(other_members_expected));
    }



    TEST_F(DsuTest, DSUDefaultConstructor)
    {
        EXPECT_EQ(HasDefaultClusters(), testing::AssertionSuccess());
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
