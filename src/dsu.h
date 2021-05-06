#ifndef __DSU_H_
#define __DSU_H_

#include <array>
#include <type_traits>
#include <iosfwd>
#include <spdlog/fmt/ostr.h>
#include <vector>
//#include <bits/c++config.h>

namespace internal {

/**
 * Converts any enum class to its underlying integral type.
 */
    template < typename E >
    auto constexpr to_integral(E e)
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    template < typename E, typename I >
    auto constexpr to_enum(I i)
    {
        return static_cast<E>(i);
    }
} //namespace internal


template < typename IndexT >
class ClusterT {
    // Basic type aliases
    using Index = IndexT;
    using Container = std::vector<Index>;

    // Data members
    Index rep {};
    Container members {};

    auto size() const { return members.size(); }

    typename Container::iterator       begin()        {  return std::begin(members);  }
    typename Container::iterator       end()          {  return std::end(members);    }
    typename Container::const_iterator cbegin() const {  return std::cbegin(members); }
    typename Container::const_iterator cend()   const {  return std::cend(members);   }

    template < typename _IndexT >
    friend std::ostream& operator<<(std::ostream& _out, ClusterT<_IndexT>&);
};

template <typename _IndexT>
inline std::ostream& operator<<(std::ostream& _out, const ClusterT<_IndexT>& cluster)
{
    _out << "{ ";
    for (auto it = cluster.begin();
         it != cluster.end();
         ++it)
    {
        _out << to_integral(cluster.rep) << ' ';
    }
    return _out << " }";
}



template < typename IndexT >
class DSU {
public:
    static_assert( std::is_enum<IndexT>::value );
    // Basic type aliases
    using Index = IndexT;
    using Cluster = ClusterT<Index>;
    using ClusterList = std::array<Cluster, to_integral(declval(IndexT::NB))>;

    static_assert( std::is_trivially_default_constructible<ClusterList>::value );

    DSU() {}

    void reset()
    {
        IndexT i = to_enum<Index>(0);
        for (Cluster& cluster : m_cl)
        {
            cluster = { to_enum<Index>(i), { to_enum<Index>(i) }};
            ++i;
        }
    }

    Index find_rep(Index ndx)
    {
        if (m_cl[ndx].rep == ndx) {
            return ndx;
        }
        return m_cl[ndx].rep = find_rep(m_cl[ndx].rep);
    }

    void unite(Index a, Index b)
    {
        a = find_rep(a);
        b = find_rep(b);

        if (a != b) {
            // Make sure the cluster at b is the smallest one
            if (m_cl[a] < m_cl[b]) {
                std::swap(a, b);
            }
                m_cl[b].rep = m_cl[a].rep;
                m_cl[a].insert(m_cl[a].cend(),
                               m_cl[b].m_cl[a].cbegin(),
                               m_cl[b].m_cl[a].cend());
                m_cl[b].m_cl[a].clear();
            }
    }

    Cluster get_cluster(Index ndx)
    {
        auto rep = find_rep(ndx);
        return { rep, m_cl[rep].members };
    }

    /**
    *  NOTE: Return a reference to the dsu array at ndx even if ndx is not the
    *   representative of its cluster (might be empty even if non-trivial cluster)
    */
    const Cluster& see_cluster(const Index ndx) const
    {
        return m_cl[ndx];
    }

    void rebase_representative(Index old_rep, Index new_rep)
    {
        m_cl[old_rep].rep = new_rep;
        m_cl[new_rep].members = m_cl[old_rep].members;
    }

    typename ClusterList::iterator begin() { return std::begin(m_cl); }
    typename ClusterList::iterator end()   { return std::end(m_cl); }

private:
    ClusterList m_cl;
};






#endif
