#ifndef __CLUSTERUTILS_H_
#define __CLUSTERUTILS_H_

#include "types.h"

namespace sg {

namespace clusters {

bool same_as_right(const Grid&, const Cell);
bool same_as_right_or_up_nbh(const Grid&, const Cell);
bool has_nontrivial_cluster(const Grid&);
Cluster get_cluster(const Grid&, const Cell);
void generate_clusters(const Grid&);
ClusterData apply_action(Grid&, const Cell);
ClusterData apply_random_action(Grid&);

} // namespace clusters

} // namespace sg


#endif
