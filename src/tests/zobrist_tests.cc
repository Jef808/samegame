
#include "samegame.h"
#include "sghash.h"
#include "zobrist.h"
#include "display.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>

namespace sg::zobrist {

namespace {


template < typename IntT, typename T >
auto count(const std::multimap<IntT, T>& mmap) {
    std::vector<std::pair<IntT, int>> ret;
    auto it = mmap.begin();
    auto it_next = it;
    while (it != mmap.end()) {
        it_next = mmap.equal_range(it->first).second;
        auto num = std::distance(it, it_next);
        ret.push_back({ it->first, num });
        it = it_next;
    }
    return ret;
}


class ZobristTest : public ::testing::Test {
protected:

    ZobristTest()
        : state(sd_root)
    {
        std::ifstream _if;
        _if.open("../data/input.txt", std::ios::in);
        if (_if)
        {
            auto tmp_state = State(_if, sd_root);
        }
        else
        {
            spdlog::error("Failed to open .../../data/input.txt\nState is initialized with empty cells");
        }
        _if.close();
    }

    void SetUp()
    {
        spdlog::default_logger()->set_level(spdlog::level::debug);
    }


    StateData sd_root {};
    State state;

    ZTable TestTable {};
};


TEST_F(ZobristTest, ZobristTableIsInitializedWith1130Elts)
{
    EXPECT_THAT(TestTable.size(), ::testing::Eq(226 * 5));
}

TEST_F(ZobristTest, ZobristTableHasDistinctEntries)
{
    std::set<Key> seen {};

    bool result = true;

    for (auto i=0; i<TestTable.size(); ++i) {
        auto [key, not_seen] = seen.insert(TestTable[i]);
        if (!not_seen) {
            spdlog::debug("Key {} is repeated.", *key);
            break;
        }
    }

    EXPECT_TRUE(result);
}

TEST_F(ZobristTest, LittleCollisionAlongRandomSimuls)
{
    std::multimap<Key, int> mmap;

    Key key = state.key();

    std::set<Key> seen {};

    ClusterData cd {CELL_NONE, Color::Empty, 2};

    int i = 0;
    mmap.insert(std::pair<Key, int>{ key, i });

    while (1) {
        ++i;
        cd = state.apply_random_action();
        if (cd.size < 2) {
            break;
        }
        key = state.key();
        mmap.insert(std::pair<Key, int> { key, i });
        auto [it, not_seen] = seen.insert(key);
        if (!not_seen) {
            spdlog::debug("\n{}\nKey = {}", state, key);
        }
    }

    auto res = 0;
    auto it = mmap.begin();
    while (it != mmap.end())
    {
        auto n = mmap.count(it->first);
        res += n-1;

        for (auto i = 0; i<n; ++i) {
            if (n > 1) {
                spdlog::debug("{} ", it->second);
            }
            ++it;
        }
    }

    EXPECT_THAT(res, ::testing::Eq(0));
}

TEST_F(ZobristTest, NoCollisionAfter10000RandomSimul)
{
    const StateData sd_backup = state.clone_data();
    bool result = true;;

    std::map<Key, StateData> seen;

    ClusterData cd {};
    Key key = 0;

    int counter = 1;

    for (int i=0; i<10000; ++i) {
        while (1) {
            ++i;
            cd = state.apply_random_action();
            ++counter;
            if (cd.size < 2) {
                break;
            }
            key = state.key();
            auto [it, not_seen] = seen.insert({ key, sd_root });

            if (!not_seen) {
                if (it->second.cells != sd_root.cells) {
                    result = false;
                    spdlog::warn("*******DUPLICATES: \n{}\nKey = {}\n{}\nKey = {}", state, it->first, it->second, it->second.key);
                }
            }
        }
        sd_root = sd_backup;
    }

    EXPECT_TRUE(result);
}


} // namespace

} // namespace sg::zobrist
