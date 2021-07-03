#ifndef __CLUSTERUTILS_H_
#define __CLUSTERUTILS_H_

#include "types.h"
#include <iosfwd>

namespace sg {

struct Grid;


namespace clusters {


/**
 * Read a grid from a file and populate the given Grid and
 * ColorCounter of the StateData object.
 */
 void input(std::istream&, Grid&, ColorCounter&);

/**
 * @Return true if the given cell has a right neighbor of the same color,
 * else false.
 */
 bool same_as_right_nbh(const Grid&, const Cell);

/**
 * @Return true if the given cell has a right neighbor or a downwards
 * neighbor of the same color, else false.
 */
 bool same_as_right_or_up_nbh(const Grid&, const Cell);

/**
 * @Return true if the grid has any cluster of size at least two,
 * else false.
 */
 bool has_nontrivial_cluster(const Grid&);

/**
 * @Return the cluster object to which the given cell belongs.
 */
 Cluster get_cluster(const Grid&, const Cell);

 ClusterData get_cluster_data(const Grid&, const Cell);
/**
 * Kill the cluster to which the given cell belongs and let the
 * remaining cells drop into the holes. Columns are then shifted
 * towards the left so that any empty column is on the right side
 * of the grid.
 *
 * @Return A cluster descriptor for the given cell.
 */
ClusterData apply_action(Grid&, const Cell);

/**
 * Same as `apply_action(Grid&, const Cell)` but a random engine
 * is used to pick the cluster at random.
 *
 * Optionally, specify a color for the random action to aim for.
 */
 ClusterData apply_random_action(Grid&, const Color = Color::Empty);

/**
 * @Return the list of valid clusters transformed into ClusterDescriptors.
 */
 ClusterDataVec get_valid_clusters_descriptors(const Grid& _g);




} // namespace clusters
} // namespace sg


#endif
