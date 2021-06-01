#ifndef __RANDOMUTILS_H_
#define __RANDOMUTILS_H_

#include <random>
#include <memory>

namespace Rand {


template < typename Int_T >
class Util {
public:
    Util() = default;
    Util(std::mt19937::result_type seed) : gen(seed) {}

    Int_T get(Int_T _min, Int_T _max)
    {
        return std::uniform_int_distribution<Int_T>(_min, _max)(gen);
    }

    auto gen_ordering(const int _beg, const int _end)
    {
        const size_t n = _end - _beg;
        std::vector<int> ret(n);
        std::iota(begin(ret), end(ret), _beg);

        for (auto i = 0; i < n; ++i) {
            Int_T j = get(i, n - 1);
            std::swap(ret[i], ret[j]);
        }

        return ret;
    }

    void shuffle(std::vector<Int_T>& v)
    {
        size_t n = v.size();

        for (auto i = 0; i < n; ++i) {
            int j = get(i, n - 1);
            std::swap(v[i], v[j]);
        }
    }

private:
    std::mt19937 gen { std::random_device{}() };
};



} // namespace sg::Random

#endif
