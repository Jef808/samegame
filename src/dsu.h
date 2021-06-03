#ifndef __DSU_H_
#define __DSU_H_

#include <array>
#include <cassert>
#include <iosfwd>
#include <iostream>
#include <numeric>
#include <memory>
#include <spdlog/fmt/ostr.h> // For fmt to recognize the operator<< of ClusterT
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

// Forward declare a template struct
template < typename Index, Index IndexNone >
struct ClusterT;

// Forward declare a template function so it is recognized inside of ClusterT
template < typename _Index, _Index _IndexNone >
std::ostream& operator<<(std::ostream&, const ClusterT<_Index, _IndexNone>&);

/**
 * A Cluster (of indices) has a `representative` index along with a container
 * containing all of its members.
 */
template <typename Index_T, Index_T IndexNone>
struct ClusterT {
    // Basic type aliases
    using Index = Index_T; // This is to make the Index type available to DSU
    static_assert(std::is_integral<Index>::value); // || std::is_enum<Index>::value);
    using value_type = Index;
    using Container = std::vector<Index>;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // Data members
    Index rep;
    Container members;

    constexpr ClusterT() : rep { IndexNone } , members(1, IndexNone)
    {
    }
    /** For reseting the DSU. */
    explicit constexpr ClusterT(Index _ndx) : rep { _ndx } , members(1, _ndx)
    {
    }
    ClusterT(Container&& _cont) : rep { IndexNone }, members(_cont)
    {
        if (!_cont.empty()) {
            rep = _cont.back();
        }
    }
    ClusterT(Index _ndx, Container&& _cont) : rep { _ndx } , members(_cont)
    {
    }

    void push_back(Index ndx) { members.push_back(ndx); }
    auto size() const { return members.size(); }

    auto begin() { return members.begin(); }
    auto end() { return members.end(); }
    auto begin() const { return members.begin(); }
    auto end() const { return members.end(); }
    auto cbegin() const { return members.begin(); }
    auto cend() const { return members.end(); }

    // Here we are declaring that template specializations of the above template operator<<
    // with parameters matching those of a ClusterT will be treated as a 'friend function'
    // of the matching ClusterT
    friend std::ostream& operator<< <> (std::ostream& _out, const ClusterT<Index, IndexNone>&);

    // Here we are telling the compiler to create a template specialization
    // of the above declared template operator== using the ClusterT's template parameters
    // in order to compare two ClusterT
    bool operator== (const ClusterT<Index, IndexNone>& other) const {
        if (size() != other.size()) {
            return false;
        }
        // Create copies so we can sort them (both 'this' and 'other' arguments are const)
        auto this_members = members;
        auto other_members = other.members;

        // Sort the members so that comparison doesn't depend on the ordering
        std::sort(this_members.begin(), this_members.end());
        std::sort(other_members.begin(), other_members.end());

        for (auto it = this_members.begin(); it != this_members.end(); ++it) {
            if (*it != other_members[std::distance(this_members.begin(), it)]) {
                return false;
            }
        }
        return true;
    }
};

 /**
 * The `Disjoint Set Union` data structure represents a partition of indices into clusters
 * using an array. To find the cluster to which some element e_{i} belongs,
 * follow e_{i+1} = array[i]. When finally e_{i+1} == i then i is the representative and the
 * whole cluster is stored there.
 */
    template < typename _Cluster, size_t N >
    class DSU {
    public:
        // Basic type aliases
        using Cluster = _Cluster;
        using Index = typename Cluster::Index;
        using Container = typename Cluster::Container;
        using ClusterList = std::array<_Cluster, N>;
        using value_type = Cluster;
        using iterator = typename ClusterList::iterator;
        using const_iterator = typename ClusterList::const_iterator;

        static inline constexpr std::array<Index, N> init = []() {
             std::array<Index, N> ret{};
             std::iota(ret.begin(), ret.end(), 0);
             return ret;
        }();

        constexpr DSU() : m_clusters( reset() ) { }
        constexpr ClusterList reset()
        {
            ClusterList ret { };
            std::transform(init.begin(), init.end(), ret.begin(), [](Index ndx) {
                return Cluster(ndx);
            });
            return ret;
        }

        // NOTE: One can also keep track of all visited nodes during the search and at the end
        // make them all point to the root (not just towards it like now).
        // But in my case, since the algorithm is probably about to kill that cluster, I don't know if it matters much
        Index find_rep(Index ndx)
        {
            assert(ndx < N);

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



            auto begin() { return m_clusters.begin(); }
            auto end() { return m_clusters.end(); }
            auto begin() const { return m_clusters.begin(); }
            auto end() const { return m_clusters.end(); }
            auto cbegin() const { return m_clusters.cbegin(); }
            auto cend() const { return m_clusters.cend(); }

    private:
        ClusterList m_clusters;
    };


// We define the template function which was forward declared at the beginnning.
// When writing _out << cluster on an instantiated ClusterT<typename T, T t> for some
// T, the compiler will find this function and know it can access cluster's private members,
// as long as "dsu.h" is included.
template <typename _Index, _Index _IndexNone>
inline std::ostream& operator<<(std::ostream& _out, const ClusterT<_Index, _IndexNone>& cluster)
{
    _out << "Rep =" << cluster.rep << " Members = {";
    for (auto m : cluster.members) {
        _out << m << ' ';
    }
    return _out << "}";
}

// The binary template function operator== acting on instance of ClusterT<T, T t> simply calls the unary operator==
// defined in the template struct.
template < typename _Index_T, _Index_T _IndexNone >
inline bool operator==(const ClusterT<_Index_T, _IndexNone>& a, const ClusterT<_Index_T, _IndexNone>& b)
{
    return ClusterT<_Index_T, _IndexNone>::operator==(b);
}



#endif
