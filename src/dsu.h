#ifndef __DSU_H_
#define __DSU_H_

#include <array>
#include <type_traits>
#include <iosfwd>
#include <spdlog/fmt/ostr.h>
#include <vector>
//#include <bits/c++config.h>

template < typename Index_T >
struct Cluster_T {
    using Index = Index_T;
        using Container = std::vector<Index_T>;
    using Cluster = Cluster_T<Index>;
        Index rep {};
        Container members {};
        bool operator<(const Cluster& other) {
            return members.size() < other.members.size();
        }
        void absorb(Cluster& other) {
            other.rep = rep;
            members.insert(members.cend(),
                               other.members.cbegin(),
                               other.members.cend());
            other.members.clear();
        }
        typename Container::iterator begin() { return std::begin(members); }
        typename Container::iterator end()   { return std::end(members); }
        typename Container::const_iterator cbegin() const { return std::cbegin(members); }
        typename Container::const_iterator cend() const { return std::cend(members); }
    template < typename _Index_T >
    friend std::ostream& operator<<(std::ostream& _out, Cluster_T<_Index_T>& cluster);
};

template <typename _Index>
inline std::ostream& operator<<(std::ostream& _out, const Cluster_T<_Index>& cluster)
    {
        _out << "{ ";
        for (auto it = cluster.begin();
             it != cluster.end();
             ++it)
        {
            _out << int(cluster.rep) << ' ';
        }
        return _out << " }";
    }
template < typename Index_T, std::size_t N >
class DSU {
public:
    static_assert(N > 0);
    static_assert(std::is_integral<Index_T>::value);

    using Index = Index_T;
    using Cluster = Cluster_T<Index>;
    using ClusterList = std::array<Cluster, N>;

public:
    DSU() { };

    DSU(DSU&& other) {
        std::swap(m_cl, other.m_cl);
    }

    DSU& operator=(DSU&& other) {
        m_cl = other.m_cl;
    }

    void reset()
    {
        auto i = 0;
        for (Cluster& cluster : m_cl)
        {
            cluster = { i, { i }};
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
            m_cl[a].absorb(m_cl[b]);
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

    void change_rep(Index old_rep, Index new_rep)
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
