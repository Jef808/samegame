/// zobrist.h
#ifndef __ZOBRIST_H_
#define __ZOBRIST_H_

#include <array>
#include <set>
#include "rand.h"

namespace zobrist {

template<typename HashFunctor, typename Key, size_t NKeys>
class KeyTable
{
 public:
  KeyTable() : m_keys(populate_keys()) {}
  template<typename... Args>
  Key operator()(Args&&... args)
  {
    return m_keys[f_hash(std::forward<Args>(args)...)];
  };
  Key operator[](size_t n) const { return m_keys[n]; }
  size_t size() const { return NKeys; }

 private:
  std::array<Key, NKeys> m_keys;
  HashFunctor f_hash{};

  std::array<Key, NKeys> populate_keys();
};

template<typename HashFunctor, typename Key, size_t N>
std::array<Key, N> KeyTable<HashFunctor, Key, N>::populate_keys()
{
  std::array<Key, N> ret{};
  Rand::Util<Key> randutil{};
  //std::set<Key> distinct_keys {};

  const auto min = std::numeric_limits<Key>::min();
  const auto max = std::numeric_limits<Key>::max();

  // while (distinct_keys.size() < N) {
  //     auto [it, inserted] = distinct_keys.insert(randutil.get(min, max));
  // }
  // std::copy(distinct_keys.begin(), distinct_keys.end(), ret.begin());

  for (auto i = 0; i < N; ++i)
  {
    ret[i] = randutil.get(min, max);
  }
  return ret;
}

// template < typename HashFunctor, typename Key, size_t NKeys >
// KeyTable<HashFunctor, Key, NKeys>::KeyTable()
//     : m_keys(populate_array<Key, N>)
// {}
// {

//     while (seen)
//     for (auto i = 0; i < NKeys; ++i) {
//         Key key = randutil.get(0, std::numeric_limits<typename Rand::template Util<Key>::size_type>::max());
//         key = key >> r;
//         key = key << r;
//         m_keys[i] = key;
//     }
// }

} // namespace zobrist

#endif
