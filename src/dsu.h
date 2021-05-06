#ifndef __DSU_H_
#define __DSU_H_

#include <array>
#include <iosfwd>
#include <spdlog/fmt/ostr.h>  // For fmt to recognize the operator<< of ClusterT
#include <type_traits>
#include <memory>
#include <utility>
#include <vector>
#include "types.h"
//#include <bits/c++config.h>


namespace np {

template < typename T, typename Param >
class NamedParameter {
public:
    explicit NamedParameter(const T& _v) : m_val(_v) {}
    explicit NamedParameter(T&& _v) : m_val(std::move(_v)) {}
    const T& get() const { return m_val; }
    T& get() { return m_val; }
private:
    T m_val;
};

template < typename IndexT >
using IndexForReset = NamedParameter< IndexT, struct ParameterForResettingDSU >;

template < typename ClusterT >
using AttachingCluster = NamedParameter< ClusterT, struct SmallerClusterOfTheTwo >;

template < typename ClusterT >
using ReceivingCluster = NamedParameter< ClusterT, struct BiggerClusterOfTheTwo >;

} // namespace np


template <typename IndexT>
struct ClusterT {
    // Basic type aliases
    using Index = IndexT;
    using Container = std::vector<Index>;

    // Data members
    Index rep { IndexT::NONE };
    Container members { {IndexT::NONE} };

    ClusterT(np::IndexForReset<IndexT> _ndx)
        : rep(_ndx.get())
        , members({_ndx.get()})
    {}

    auto size() const { return members.size(); }

    typename Container::iterator begin() { return std::begin(members); }
    typename Container::iterator end() { return std::end(members); }
    typename Container::const_iterator cbegin() const { return std::cbegin(members); }
    typename Container::const_iterator cend() const { return std::cend(members); }

    template <typename _IndexT>
    friend std::ostream& operator<<(std::ostream& _out, ClusterT<_IndexT>&);
};




template <typename IndexT>
class DSU {
public:
    static_assert(std::is_enum<IndexT>::value);
    // Basic type aliases
    using Index = IndexT;
    static constexpr const std::size_t Largest_Index = to_integral(IndexT::NB);
    using Cluster = ClusterT<Index>;
    using ClusterList = std::array<Cluster, Largest_Index>;

    static_assert(std::is_trivially_default_constructible<ClusterList>::value);

    DSU() { }

    void reset()
    {
        Index ndx = to_enum<Index>(0);
        auto reset_cluster = [&ndx](Cluster& cl) mutable {
            cl = Cluster(np::IndexForReset(ndx));
        };

        std::for_each(m_clusters.begin(), m_clusters.end(), reset_cluster);
    }

    Index find_rep(Index ndx)
    {
        bool not_the_rep = m_clusters[ndx].rep != ndx;

        if (not_the_rep)
        {
            return m_clusters[ndx].rep = find_rep(m_clusters[ndx].rep);
        }

        return ndx;
    }

    /**
     * Append a cluster at the end of another one.
     *
     * NOTE: It is important to always merge the smaller cluster INTO the bigger cluster
     * for this type of data structure. The depth of any component stays in control
     * and consequently the find_rep queries are more efficient.
     */
    void merge_clusters(np::ReceivingCluster<Cluster> _larger_a, np::AttachingCluster<Cluster> _smaller_b)
    {
        // The index at b now points to the representative of cluster a
        _smaller_b.rep = _larger_a.rep;

        // Insert the new cells
        _larger_a.insert(_larger_a.cend(),
                             _smaller_b.begin(),
                             _smaller_b.end());

        // Clean up the merged cluster (only representatives have non-empty clusters at their index)
        _smaller_b.members.clear();
    }

    void unite(Index a, Index b)
    {
        a = find_rep(a);
        b = find_rep(b);

        if (a != b)
        {
            if (m_clusters[a].size() < m_clusters[b].size()) {
                std::swap(a, b);
            }
            // Now a is always the bigger cluster of the two (or they are equal in size).
            merge_clusters(a, b);
        }
    }

    /**
     * Of course the clusters are temporary objects, we use shared pointers to
     * let 'users' burrow a cluster while preventing memory leaks.
     */
    std::shared_ptr<Cluster> get_cluster(Index ndx)
    {
        auto rep = find_rep(ndx);
        query_ledger.push_back(std::make_shared<Cluster>(m_clusters[rep]));
        std::shared_ptr<Cluster> ret = query_ledger.back();
        return ret;
    }

    typename ClusterList::iterator begin() { return std::begin(m_clusters); }
    typename ClusterList::iterator end() { return std::end(m_clusters); }

private:
    ClusterList m_clusters;
    std::vector<std::shared_ptr<Cluster>> query_ledger;
};

template <typename _IndexT>
inline std::ostream& operator<<(std::ostream& _out, const ClusterT<_IndexT>& cluster)
{
    _out << "{ ";
    for (auto it = cluster.begin();
         it != cluster.end();
         ++it) {
        _out << to_integral(cluster.rep) << ' ';
    }
    return _out << " }";
}

#endif
