
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "spdlog/spdlog.h"
#include "rand.h"
#include <map>
#include <utility>

namespace Rand {

namespace {

template < typename IntT >
auto counter(const std::multimap<IntT, int>& mmap) {
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

class RandUtilTest : public ::testing::Test {
protected:
    RandUtilTest()
    {
        spdlog::default_logger()->set_level(spdlog::level::debug);
    }

    template < typename IntT >
    struct LIMIT {
        static const IntT MIN = std::numeric_limits<IntT>::min();
        static const IntT MAX = std::numeric_limits<IntT>::max();
    };
};


TEST_F(RandUtilTest, NumberOfCollisionsAsExpected8Bit)
{
    using Int_T = uint8_t;
    Util<Int_T> rand_util {};
    std::multimap<Int_T, int> buckets;

    for (int i=0; i<1000; ++i) {
        buckets.insert({ rand_util.get(LIMIT<Int_T>::MIN, LIMIT<Int_T>::MAX), i});
    }

    auto collisions = counter(buckets);
    auto biggest_clash = std::max_element(collisions.begin(), collisions.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    EXPECT_THAT(collisions.size(), ::testing::Le(1000));
    EXPECT_THAT(biggest_clash->second, ::testing::Ge(1));
}

TEST_F(RandUtilTest, NumberOfCollisionsAsExpected32Bit)
{
    using Int_T = uint32_t;
    Util<Int_T> rand_util {};
    std::multimap<Int_T, int> buckets;

    for (int i=0; i<1000; ++i) {
        buckets.insert({ rand_util.get(LIMIT<Int_T>::MIN, LIMIT<Int_T>::MAX), i});
    }

    auto collisions = counter(buckets);
    auto biggest_clash = std::max_element(collisions.begin(), collisions.end(), [](const auto& a, const auto& b) {
        if (a.second > 1)
        {
            spdlog::debug("Key = {} ----> {} duplicates", a.first, a.second);
        }
        return a.second < b.second;
    });

    EXPECT_THAT(collisions.size(), ::testing::Eq(1000));
    EXPECT_THAT(biggest_clash->second, ::testing::Eq(1));
}

TEST_F(RandUtilTest, NumberOfCollisionsAsExpected64Bit)
{
    using Int_T = uint64_t;

    Util<Int_T> rand_util {};
    std::multimap<Int_T, int> buckets;

    for (int i=0; i<1125; ++i) {
        buckets.insert({ rand_util.get(LIMIT<Int_T>::MIN, LIMIT<Int_T>::MAX), i});
        //spdlog::debug("Key = {}, success = {}", it->first, it->second);
    }

    auto collisions = counter(buckets);
    auto biggest_clash = std::max_element(collisions.begin(), collisions.end(), [](const auto& a, const auto& b) {
        if (a.second > 1)
        {
            spdlog::debug("Key = {} ----> {} duplicates", a.first, a.second);
        }
        return a.second < b.second;
    });

    EXPECT_THAT(collisions.size(), ::testing::Eq(1125));
    EXPECT_THAT(biggest_clash->second, ::testing::Eq(1));
}




} // namespace
} // namespace Rand
