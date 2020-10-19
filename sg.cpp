#include <bits/stdc++.h>
#include <unistd.h>

using namespace std;

#define WIDTH 15
#define HEIGHT 15
#define NB_CELLS 225
#define NB_COLORS 5

#define BEAM_SIZE 225

using int8 = uint8_t;
using grid = array<int8, NB_CELLS>;

// ----------- DISJOINT SET UNION STRUCTURE ------------------

class DSU
{
public:
    grid parent{0};
    array< vector<int8>, NB_CELLS > members;

    DSU();

    void make_set(int8);
    int8 find_rep(int8);
    void unite(int8, int8);
    void reset();
};

DSU::DSU(): parent{0}
{
    reset();
}

void DSU::make_set(int8 v)
{
	parent.at(v) = v;
	members.at(v) = vector<int8>(1, v);
}

int8 DSU::find_rep(int8 v)
{
	if (parent.at(v) == v) {
		return v;
	}
	return parent.at(v) = find_rep(parent.at(v));
}

void DSU::unite(int8 a, int8 b)
{
	a = find_rep(a);
	b = find_rep(b);
	if (a != b) {
		if (members.at(a).size() < members.at(b).size()) {
			swap(a, b);
		}
		while (!members.at(b).empty()) {
			int8 v = members.at(b).back();
			members.at(b).pop_back();
			parent[v] = a;
			members.at(a).push_back(v);
		}
	}
}

void DSU::reset()
{
    for (int i=0; i<NB_CELLS; ++i) {
            make_set(i);
    }
}


// ----------------------  STATE  ------------------------- //

struct StateNode
{
    grid data{0};
    array<int8, NB_COLORS+1> clr_counter{0};
    array<vector<int8>, BEAM_SIZE> unvisited_children;  // Potential moves
    int n_unvisited_children{0};                        // Number of potential moves
    StateNode* parent_state;
    array<unique_ptr<StateNode>, BEAM_SIZE> children;   // Expanded nodes
    int n_children{0};                                  // Number of expanded nodes
    queue<int8> best_actions;                           // Best sequence of moves to reach a terminal node
    int highest_score{0};                               // Highest score of node to date
    StateNode(grid&, array<int8, NB_COLORS+1>&);        // Constructor to initialize
    StateNode(grid&, array<int8, NB_COLORS+1>&, StateNode*, int);  // Constructor for adding child
    StateNode(StateNode&);                              // Copy constructor
};

StateNode::StateNode(grid& state, array<int8, NB_COLORS+1>& clr_count): data{state}, clr_counter{clr_count}, n_unvisited_children{0}, n_children{0},
    parent_state(this), highest_score{0}
{

}

StateNode::StateNode(grid& state, array<int8, NB_COLORS+1>& clr_count, StateNode* parent, int score):
    data{state}, clr_counter{clr_count}, n_unvisited_children{0}, n_children{0},
    parent_state(parent), highest_score{score}
{

}

StateNode::StateNode(StateNode& state):
    data{state.data}, clr_counter{state.clr_counter}, n_unvisited_children{state.n_unvisited_children},
    n_children{0}, parent_state(state.parent_state), highest_score{state.highest_score},
    unvisited_children{state.unvisited_children}
{

}


// ---------------------  SIMULATION  --------------------------- //

void step(grid&, const vector<int8>&);
bool is_done(grid&);
bool is_cleared(grid&);


void destroy_group(grid& state, const vector<int8>& group)
{
    for (int8 ndx: group) {
        state.at(ndx) = 0;
    }
}

void do_gravity(grid &state)
{
	// For all columns
	for (int i=0; i<WIDTH; ++i) {
		int bottom = i + WIDTH * (HEIGHT - 1);
		// look for an empty cell
		while (bottom>WIDTH-1 && state[bottom]!=0) {
			bottom -= WIDTH;
		}
		// make non-empty cells above drop
		int top = bottom - WIDTH;
		while (top>-1) {
			// if top is non-empty, swap its color with empty color and move both pointers up
			if (state[top]!=0) {
				swap(state[top], state[bottom]);
				bottom -= WIDTH;
			}
			// otherwise move top pointer up
			top -= WIDTH;
		}
	}
}

void pull_columns(grid& state)
{
	int left_col = WIDTH * (HEIGHT - 1);
	// look for an empty column
	while (left_col<NB_CELLS && state[left_col]!=0) {
		++left_col;
	}
	// pull columns on the right to the empty column
	int right_col = left_col + 1;
	while (right_col < NB_CELLS) {
		// if the column is non-empty, swap all its colors with the empty column and move both pointers right
		if (state[right_col]!=0) {
			for (int i=0; i<HEIGHT; ++i) {
				swap(state[right_col - i * WIDTH], state[left_col - i * WIDTH]);
			}
			++left_col;
		}
		// otherwise move right pointer right
		++right_col;
	}
}

void step(grid& state, const vector<int8>& group)
{
	destroy_group(state, group);
	do_gravity(state);
	pull_columns(state);
}

void render(grid& state, int action)
{
    for( int y=0; y<HEIGHT; ++y) {
        for( int x=0; x<WIDTH; ++x) {
            int color_code{30};
            int cell_clr = (int)state.at(x + y * WIDTH);
            if (cell_clr != 0)
                color_code = 90 + cell_clr;
            if (x + y * WIDTH == action) {
                cout << "\033[" << color_code << "m\u25A1 ";
            } else {
                cout << "\033[" << color_code << "m\u25A0 ";
            }
        }
        cout << endl;
    }
    cout << "\033[0m" << endl;
}

bool is_done(StateNode& state)
{
    return all_of(state.clr_counter.begin(), state.clr_counter.end(),
                  [](const int clr_count) { return clr_count < 2; });
}

bool is_cleared(StateNode& state)
{
    return all_of(state.clr_counter.begin(), state.clr_counter.end(),
                  [](const int clr_count) { return clr_count == 0; });
}


// -----------------------  AGENTS  ------------------------------ //

class Agent: public DSU
{
public:
    StateNode& root;

    int score_move(const vector<int8>&);      // (n-2)^2 where n is size of group
    void get_groups(const grid&);             // Generates the groups using DSUF algorithm
    void set_unvisited_children(StateNode&);  // Sets the potential moves in the state
    void add_child(StateNode&);               // Expand a child node

    Agent(StateNode*);                        // Initialize agent with pointer to root
};

Agent::Agent(StateNode* state): DSU(), root(*state)
{
    set_unvisited_children(this->root);
}

int Agent::score_move(const vector<int8>& group)
{
    if (group.size() < 3) { return 0; }
    return (group.size() - 2) * (group.size() - 2);
}

void Agent::get_groups(const grid& state)
{
	/* Use Disjoint Union Find structure to separate the cells into clusters.
	 * We run through the grid from left to right, comparing with cells above and right
	 */
	// First row (nothing above)
	DSU::reset();
	for (int i=0; i<WIDTH-1; ++i) {
		if (state.at(i+1)==0 || state.at(i+1)!=state.at(i)) continue;
		DSU::unite(i, i+1);
	}
	for (int i=WIDTH; i<NB_CELLS; ++i) {
		// above
		if (state.at(i-WIDTH)!=0 && state.at(i-WIDTH)==state.at(i))
			DSU::unite(i, i-WIDTH);
		// right
		if (i%WIDTH==(WIDTH-1) || state.at(i+1)==0 || state.at(i+1)!=state.at(i)) continue;
		DSU::unite(i, i+1);
	}
}

void Agent::set_unvisited_children(StateNode& state)
{
    /* Populates Agent::unvisited_children with `BEAM_SIZE` groups to explore */
    vector<vector<int8>> nontrivial_groups;
    get_groups(state.data);
    copy_if(this->members.begin(), this->members.end(),
            back_inserter(nontrivial_groups),
            [](vector<int8> grp) { return grp.size() > 1; }
    );
    // 
    state.n_unvisited_children = min((int)nontrivial_groups.size(), BEAM_SIZE);

    partial_sort_copy(nontrivial_groups.begin(), nontrivial_groups.end(),
                      state.unvisited_children.begin(), state.unvisited_children.end(),
            [&state](const vector<int8>& grp1, const vector<int8>& grp2)
            {
                if (state.data.at(grp1.back()) == state.data.at(grp2.back())) {
                    return ( grp1.size() < grp2.size() );
                } else {
                    return ( state.clr_counter.at(state.data.at(grp1.back())) < state.clr_counter.at(state.data.at(grp2.back())) ); 
                }
            }
    );
}

void Agent::add_child(StateNode& state)
{
    vector<int8>& group = state.unvisited_children.at(state.n_children);
    grid child_state = state.data;
    array<int8, NB_COLORS+1> child_clr_counter = state.clr_counter;
    child_clr_counter.at(state.data.at(group.back())) -= group.size();
    step(child_state, group);

    int score = state.highest_score;
    auto child_node = make_unique<StateNode>(child_state, child_clr_counter, &state, score_move(group));
    set_unvisited_children(*child_node);
    state.children.at(state.n_children) = move(child_node);
}


class AgentBeam: public Agent
{
public:
    void beam_search_actions(StateNode&, queue<int8>&);
    void find_best_actions();
    int choose_action();
    int rollout_small_clr_first(StateNode&);

    AgentBeam(StateNode*);
};

AgentBeam::AgentBeam(StateNode* state): Agent{state}
{

}

int AgentBeam::rollout_small_clr_first(StateNode& state)
{
    /* Serves as a heuristic for the maximum score */
    if (state.n_unvisited_children == 0) {
        if (is_cleared(state)) {
            state.highest_score += 1000;
        }
        return state.highest_score;
    }
    add_child(state);
    StateNode& child = *state.children.at(0);
    int child_score = child.highest_score + rollout_small_clr_first(child);
    return child_score;
}

void AgentBeam::beam_search_actions(StateNode& state, queue<int8>& actions)
{
    /* Rollout every child (with pruning?) to find the best path
    */

    if (state.n_unvisited_children == 0) { return; }
    int best_score{0};
    int best_child{0};
    for (int i=0; i<state.n_unvisited_children; ++i) {
        add_child(state);
        ++state.n_children;
        StateNode child(*state.children.at(i));
        int child_score = rollout_small_clr_first(child);
        if (child_score > best_score) {
            best_score = child_score;
            best_child = i;
        }
    }
    state.highest_score = best_score;
    actions.push(state.unvisited_children.at(best_child).back());
    beam_search_actions(*state.children.at(best_child), actions);
}

void AgentBeam::find_best_actions()
{
    beam_search_actions(this->root, this->root.best_actions);
}


int AgentBeam::choose_action()
{
    /* Return best action, -1 if over */
    if (this->root.best_actions.empty()) {
        return -1;
    }
    int action = this->root.best_actions.front();
    this->root.best_actions.pop();
    return action;
}

// ------------------------  INPUT/MAIN  ------------------------- //

void get_input(grid&, array<int8, NB_COLORS+1>&);

void get_input(grid& grid_container, array<int8, NB_COLORS+1>& clr_counter_container)
{
	for (int i=0; i<NB_CELLS; ++i) {
        int clr_buffer{0};
		cin >> clr_buffer;
        grid_container.at(i) = ++clr_buffer;
        ++(clr_counter_container.at(clr_buffer));  // increment grid so that empty cell is 0
	}
}


int main()
{
    ios_base::sync_with_stdio(false); cin.tie(NULL);

#ifndef ONLINE_JUDGE
    freopen("input.txt", "r+", stdin);
    // freopen("error.txt", "w+", stderr);
    // freopen("output.txt", "w+", stdout);
#endif

    // Create an 'empty' array and populate it with the turn input.
    grid state{0};
    array<int8, NB_COLORS+1> clr_counter{0};
    get_input(state, clr_counter);
    StateNode* state_node = new StateNode(state, clr_counter);
    AgentBeam agent(state_node);
    // Generate action sequence
    agent.find_best_actions();
    int action = agent.choose_action();
    while (!agent.root.best_actions.empty()) {
        cout << action << ' ';
        action = agent.choose_action();
    }
    cout << endl;
    cout << "SCORE : " << agent.root.highest_score << endl;

    cerr << "time taken : " << (float)clock() / CLOCKS_PER_SEC << " secs" << endl;
    return 0;
}