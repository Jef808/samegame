#ifndef __RANDOMUTILS_H_
#define __RANDOMUTILS_H_

#include <array>
#include <type_traits>
#include <random>
#include <memory>
#include <vector>

namespace Rand {

template<typename Int_T>
class Util
{
 public:
  using Engine = typename std::
      conditional<sizeof(Int_T) <= 4, std::mt19937, std::mt19937_64>::type;

  Util() = default;
  Util(typename Engine::result_type seed) : gen(seed) {}
  using size_type = typename std::make_unsigned<Int_T>::type;

  Int_T get(Int_T _min, Int_T _max)
  {
    return std::uniform_int_distribution<Int_T>(_min, _max)(gen);
  }

  auto gen_ordering(const Int_T _beg, const Int_T _end)
  {
    const size_type n = _end - _beg;
    std::vector<Int_T> ret(n);
    std::iota(begin(ret), end(ret), _beg);

    for (auto i = 0; i < n; ++i)
    {
      auto j = get(i, n - 1);
      std::swap(ret[i], ret[j]);
    }

    return ret;
  }

  void shuffle(std::vector<Int_T>& v)
  {
    size_type n = v.size();

    for (auto i = 0; i < n; ++i)
    {
      auto j = get(i, n - 1);
      std::swap(v[i], v[j]);
    }
  }

 private:
  Engine gen{std::random_device{}()};
};

} // namespace Rand

#endif
