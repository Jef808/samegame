#ifndef __DSU_H_
#define __DSU_H_

#include <array>
#include <iosfwd>
#include <memory>
#include <spdlog/fmt/ostr.h> // For fmt to recognize the operator<< of ClusterT
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

namespace sg {

/**
 * A Cluster (of indices) has a `representative` index along with a container
 * containing all of its members.
 */
template <typename Index_T, Index_T DefaultValue = 0>
struct ClusterT {
    // Basic type aliases
    using Index = Index_T; // This is to make the Index type available to DSU
    static_assert(std::is_integral<Index>::value); // || std::is_enum<Index>::value);
    using Container = std::vector<Index>;

    // Data members
    Index rep;
    Container members;

    ClusterT()
        : rep { DefaultValue }
        , members { { DefaultValue } }
    {
    }

    /** For reseting the DSU. */
    explicit ClusterT(Index _ndx)
        : rep(_ndx)
        , members({ _ndx })
    {
    }

    ClusterT(Index _ndx, Container&& _cont)
        : rep(_ndx)
        , members(_cont)
    {
    }

    void append(Index ndx) { members.push_back(ndx); }

    auto size() const { return members.size(); }

    typename Container::iterator begin() { return std::begin(members); }
    typename Container::iterator end() { return std::end(members); }
    typename Container::const_iterator cbegin() const { return std::cbegin(members); }
    typename Container::const_iterator cend() const { return std::cend(members); }

    template <typename _Index, _Index _DefaultValue>
    friend std::ostream& operator<<(std::ostream& _out, const ClusterT<Index, _DefaultValue>&);
};

namespace details {

    /**
 * The `Disjoint Set Union` data structure represents a partition of indices into clusters
 * using an array. To find the cluster to which some element e_{i} belongs,
 * follow e_{i+1} = array[i]. When finally e_{i+1} == i then i is the representative and the
 * whole cluster is stored there.
 */
    template <typename Cluster, size_t N>
    class DSU {
    public:
        // Basic type aliases
        using Index = typename Cluster::Index;
        using Container = typename Cluster::Container;
        using ClusterList = std::array<Cluster, N>;

        DSU()
            : m_clusters { Cluster() }
        {
        }

        void reset()
        {
            Index ndx = 0;
            auto reset_cluster = [&ndx](Cluster& cl) mutable {
                cl = Cluster(ndx);
                ++ndx;
            };

            std::for_each(m_clusters.begin(), m_clusters.end(), reset_cluster);
        }

        // NOTE: One can also keep track of all visited nodes during the search and at the end
        // make them all point to the root (not just towards it like now).
        // But in my case, since the algorithm is probably about to kill that cluster, I don't know if it matters much
        Index find_rep(Index ndx)
        {
            bool not_the_rep = m_clusters[ndx].rep != ndx;

            if (not_the_rep) {
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
        void merge_clusters(Cluster& _larger_a, Cluster& _smaller_b)
        {
            // Make the representative of the smaller cluster now point to the representative of the bigger one.
            _smaller_b.rep = _larger_a.rep;
            // Insert the new cells
            _larger_a.members.insert(_larger_a.cend(),
                _smaller_b.begin(),
                _smaller_b.end());

            // Clean up the merged cluster (only representatives have non-empty clusters at their index)
            _smaller_b.members.clear();
        }

        void unite(Index a, Index b)
        {
            a = find_rep(a);
            b = find_rep(b);

            if (a != b) {
                if (m_clusters[a].size() < m_clusters[b].size()) {
                    std::swap(a, b);
                }
                // Now a is always the bigger cluster of the two (or they are equal in size).
                merge_clusters(m_clusters[a], m_clusters[b]);
            }
        }

        /**
     * Return a copy of the cluster containing index ndx.
     */
        Cluster get_cluster(Index ndx)
        {
            auto rep = find_rep(ndx);
            return m_clusters[rep];
        }

        typename ClusterList::iterator begin() { return m_clusters.begin(); }
        typename ClusterList::iterator end() { return m_clusters.end(); }
        typename ClusterList::const_iterator cbegin() const { return m_clusters.cbegin(); }
        typename ClusterList::const_iterator cend() const { return m_clusters.cend(); }

    private:
        ClusterList m_clusters;
    };

} // namespace details

// template < typename _Index >
// inline std::string to_string(const typename ClusterT<_Index>::Container& cl_members) {
//     std::stringstream ss;

//     ss << "{ ";
//     for (auto it = cl_members.cbegin();
//          it != cl_members.cend();
//          ++it)
//     {
//         ss << *it << ' ';
//     }
//     ss << " }";

//     return ss.str();
// }

// template < typename _Index >
// inline std::ostream& operator<<(std::ostream& _out, const typename ClusterT<_Index>::Container& cl_members) {
//      return _out << to_string(cl_members);
//}

template <typename _Index, _Index _DefaultValue>
inline std::ostream& operator<<(std::ostream& _out, const ClusterT<_Index, _DefaultValue>& cluster)
{
    _out << "Rep=" << cluster.rep << " { ";
    for (auto it = cluster.members.cbegin();
         it != cluster.members.cend();
         ++it) {
        _out << *it << ' ';
    }

    return _out << " }";
}

/**
 * When comparing clusters, only look at the (unordered) members,
 * the representatives don't matter as long as the members are the same.
 */
template <typename _Index_T, _Index_T _DefaultValue>
inline bool operator==(const ClusterT<_Index_T, _DefaultValue>& a, const ClusterT<_Index_T, _DefaultValue>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    // Sort to make the two containers comparable
    // NOTE: Need to copy them first since they are passed
    // by const ref
    auto a_cpy = a;
    auto b_cpy = b;

    std::sort(a_cpy.members.begin(), a_cpy.members.end());
    std::sort(b_cpy.members.begin(), b_cpy.members.end());

    for (auto i = 0; i < a.members.size(); ++i) {
        if (a_cpy.members[i] != b_cpy.members[i]) {
            return false;
        }
    }
    return true;
}

} // namespace sg

#endif
