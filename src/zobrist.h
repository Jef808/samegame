/// zobrist.h
#ifndef __ZOBRIST_H_
#define __ZOBRIST_H_

#include <array>
#include "rand.h"

namespace zobrist {


template < typename HashFunctor, typename Key, std::size_t NKeys >
class KeyTable {
public:
    KeyTable(uint = 0);
    template < typename ...Args >
    Key operator()(Args&&... args) {
        return f_hash(std::forward<Args>(args)...);
    };

private:
    std::array<Key, NKeys> m_keys;
    HashFunctor f_hash;
};

template < typename HashFunctor, typename Key, std::size_t NKeys >
KeyTable<HashFunctor, Key, NKeys>::KeyTable(uint _n_bits_reserved)
{
    Rand::Util<Key> randutil {};
    for (auto i = 0; i < NKeys; ++i) {
        Key key = randutil.get(std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
        m_keys[i] = _n_bits_reserved << (key >> _n_bits_reserved);
    }
}

} // namespace zobrist


#endif
