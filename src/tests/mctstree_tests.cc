/// Tests for the MctsTree datastructure
#include "mctstree.h"
#include "types.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <algorithm>
#include <fstream>
#include <type_traits>


namespace mctsimpl {

namespace {

class MctsTreeTest : public ::testing::Test {
protected:
    MctsTreeTest()
        : m_tree()
    {
    }
    void SetUp()
    {
    }
    const sg::ClusterData CD_NONE = { .rep=sg::CELL_NONE, .color=sg::Color::Empty, .size=0 };
    MctsTree m_tree;
};

class MctsTreeBasicTest : public MctsTreeTest {
protected:
    MctsTreeBasicTest()
        : m_test_children()
    {
        p_root = m_tree.set_root(1);
    }
     
    Node* p_root;
    std::vector<Edge> m_test_children;
};

TEST_F(MctsTreeTest, EdgesAreDefaultConstructible)
{
    EXPECT_THAT(std::is_default_constructible<Edge>::value, ::testing::IsTrue());
}

TEST_F(MctsTreeTest, NodesAreDefaultConstructible)
{
    EXPECT_THAT(std::is_default_constructible<Node>::value, ::testing::IsTrue());
}

TEST_F(MctsTreeTest, TreeIsDefaultConstructible)
{
    EXPECT_THAT(std::is_default_constructible<MctsTree>::value, ::testing::IsTrue());
}

TEST_F(MctsTreeBasicTest, OneNodeAfterSetRoot)
{
    EXPECT_THAT(m_tree.size(), ::testing::Eq(1));
}

TEST_F(MctsTreeBasicTest, SetRootStoresTheKey)
{
    EXPECT_THAT(p_root->key, ::testing::Eq(1));
}

TEST_F(MctsTreeBasicTest, OneNodeAfterSetRootTwice)
{
    m_tree.set_root(1);
    EXPECT_THAT(m_tree.size(), ::testing::Eq(1));
}

TEST_F(MctsTreeBasicTest, SetChildrenCopiesTheChildren)
{
    std::vector<Edge> _children {};
}



} // namespace

} // namespace mctsimpl
