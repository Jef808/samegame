// samegame.cpp
//
// TODO Don't store as much data for the states. Maybe declare the
//
//
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <functional>
#include <list>
#include <map>
#include <random>
#include <utility>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include "samegame.h"

//#include "debug.h"


namespace sg {

const Cluster CLUSTER_NONE = {CELL_NONE, {}};

std::string to_string(const std::list<Cell> cl_members) {
    std::stringstream ss;
    for (const auto& m : cl_members) {
        ss << m << ", ";
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& _out, const Cluster& cluster) {
    return _out << (cluster == CLUSTER_NONE ? "CLUSTER_NONE" : to_string(cluster.members));
}

//**************************  Disjoint Union Structure  ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
namespace DSU {

    // using ClusterList = std::array<Cluster, MAX_CELLS>;
    //using repCluster = std::pair<Cell, Cluster>;
    //typedef std::array<repCluster, MAX_CELLS> ClusterList;

    ClusterList cl { };

    void reset()
    {
        Cell cell = 0;
        for (auto& c : cl) {
            c.rep = cell;
            c.members = { cell };
            ++cell;
        }
    }

    Cell find_rep(Cell cell)
    {
        // Condition for cell to be a representative
        if (cl[cell].rep == cell) {
            return cell;
        }
        return  cl[cell].rep = find_rep(cl[cell].rep);
    }

    void unite(Cell a, Cell b)
    {
        a = find_rep(a);
        b = find_rep(b);

        if (a != b) {
             // Make sure the cluster at b is the smallest one
            if (cl[a].members.size() < cl[b].members.size()) {
                std::swap(a, b);
            }

            cl[b].rep = cl[a].rep; // Update the representative
            cl[a].members.splice(cl[a].members.end(), cl[b].members); // Merge the cluster of b into that of a
        }
    }

} //namespace DSU

//*************************************  Zobrist  ***********************************/

namespace Zobrist {

    std::array<Key, 1126> ndx_keys { 0 }; // First entry = 0 for a trivial action

    Key cellColorKey(Cell cell, Color color) {
        return ndx_keys[cell * color];
    }

    Key clusterKey(const ClusterData& cd) {
        return ndx_keys[cd.rep * cd.color];
    }
    // Key rarestColorKey(const State& state) {
    //     auto rarest_color = *std::min_element(state.colors.begin() + 1, state.colors.end());
    //     return rarest_color << 1;
    // }
    Key terminalKey(const State& state) {
        return state.is_terminal();
    }

} // namespace Zobrist

//********************************  Bitwise methods  ***********************************/

// Generate a Zobrist hash from the grid
// TODO: As for generate_clusters(), can be more efficient
Key State::generate_key() const
{
    generate_clusters();

    Key key = 0;
    uint8_t ndx = 0;

    for (auto cell : p_data->cells) {
        if (cell != COLOR_NONE) {
            key ^= Zobrist::cellColorKey(ndx, cell);
        }
        ++ndx;
    }

    key ^= Zobrist::terminalKey(*this);

    return key;

    // for (const auto& cluster : r_clusters)
    // {
    //     if (cluster.members.size() > 1)
    //     {
    //         //auto rep = *std::min_element(begin(cluster.members), end(cluster.members));
    //         key ^= Zobrist::clusterKey({cluster.rep, p_data->cells[cluster.rep]});
    //     }
    // }

    //key ^= Zobrist::rarestColorKey(*this);
    //key ^= Zobrist::terminalKey(*this);
}

Key State::generate_key(const Cluster& cluster) const
{
    if (cluster.members.size() < 2)
        return 0;

    Color cluster_color = p_data->cells[cluster.members.front()];
    Key key = 0;
    for (auto cell : cluster.members)
        key ^= Zobrist::cellColorKey(cell, cluster_color);

    return key;
}

// Key State::apply_action(Key state_key, Key action_key)
// {
//     return 0;
// }

Key State::key() const
{
    // if (p_data->key == 0)
    //     p_data->key = generate_key();
    return p_data->key;
}

//*************************************** Game logic **********************************/

void State::init()
{
    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max());

    for (int i = 1; i < 1126; ++i) {
        Zobrist::ndx_keys[i] = ((dis(rd) >> 1) << 1); // Reserve first bit for terminal
    }
}

State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
    , colors{0}
    , r_clusters(DSU::cl)
{
    int _in_color = 0;
    Grid& cells = p_data->cells;

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            _in >> _in_color;
            cells[j + i * WIDTH] = Color(_in_color + 1);
        }
    }

    generate_clusters();
    init_ccounter();
    p_data->key = generate_key();
    p_data->ply = 0;
}

void State::init_ccounter()
{
    Grid& m_cells = p_data->cells;
    for (const auto& cluster : r_clusters)
    {
        colors[m_cells[cluster.members.back()]] += cluster.members.size();
    }
}

// TODO: Only check columns intersecting the killed cluster
void State::pull_cells_down()
{
    Grid& m_cells = p_data->cells;
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { COLOR_NONE };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j) {
            auto bottom_color = m_cells[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != COLOR_NONE) {
                new_column[new_height] = bottom_color;
                ++new_height;
            }
        }
        // pop back the value (including padding with 0)
        for (int j = 0; j < HEIGHT; ++j) {
            m_cells[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
        }
    }
}

// TODO: Only check columns (before finding one) intersecting the killed cluster
void State::pull_cells_left()
{
    Grid& m_cells = p_data->cells;
    int i = 0;
    std::deque<int> zero_col;

    while (i < WIDTH)
    {
        if (zero_col.empty())
        {
            // Look for empty column
            while (i < WIDTH - 1 && m_cells[i + (HEIGHT - 1) * WIDTH] != COLOR_NONE) {
                ++i;
            }
            zero_col.push_back(i);
            ++i;

        } else
        {
            int x = zero_col.front();
            zero_col.pop_front();
            // Look for non-empty column
            while (i < WIDTH && m_cells[i + (HEIGHT - 1) * WIDTH] == COLOR_NONE) {
                zero_col.push_back(i);
                ++i;
            }
            if (i == WIDTH)
                break;
            // Swap the non-empty column with the first empty one
            for (int j = 0; j < HEIGHT; ++j) {
                std::swap(m_cells[x + j * WIDTH], m_cells[i + j * WIDTH]);
            }
            zero_col.push_back(i);
            ++i;
        }
    }
}

bool State::is_terminal() const
{
    return is_terminal(p_data->key);
}

bool State::check_terminal() const
{
    auto _beg = r_clusters.begin();
    auto _end = r_clusters.end();

    return std::none_of(_beg, _end, [](const auto& cluster) { return cluster.members.size() > 1; });
}

bool State::is_empty() const
{
    return cells()[(HEIGHT - 1) * WIDTH] == COLOR_NONE;
}


// NOTE: Can return empty vector
ActionDVec State::valid_actions_data() const
{
    generate_clusters();

    ActionDVec ret { };

    for (auto& cluster : r_clusters) {
        if (uint8_t sz = cluster.members.size(); sz > 1) {
            if (Cell rep = cluster.members.front(); rep != CELL_NONE) {
                if (Color color = p_data->cells[rep]; color != COLOR_NONE) {
                    ret.emplace_back(ClusterData{rep, color, sz});
                }
            }
        }
    }
    return ret;
}

//******************************* MAKING MOVES ***********************************/


// TODO start from bottom left corner and try to stop early when most of the grid is empty.
void State::generate_clusters() const
{
    // TODO: Implement something like that:
    // if (!need_refresh_clusters) {
    //     spdlog::debug("Unnecessary gen_clusters call");
    //     return;
    // }
    DSU::reset();
    Grid& m_cells = p_data->cells;
    // First row
    for (int i = 0; i < WIDTH - 1; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare right
        if (m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }
    // Other rows
    for (int i = WIDTH; i < MAX_CELLS; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare up
        if (m_cells[i - WIDTH] == m_cells[i])
            DSU::unite(i - WIDTH, i);
        // Compare right if not at end of row
        if ((i + 1) % WIDTH != 0 && m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }

    // Always consider the the smallest representative for a given cluster
    // to get a bijection (we want to use them to generate keys)
    for (auto& cluster : DSU::cl)
    {
        cluster.members.sort();
        cluster.rep = cluster.members.front();
    }
}

// TODO Stop sending back naked pointers to data that's constantly changing maybe?
Cluster* State::get_cluster(Cell cell) const
{
    generate_clusters();
    if (cell == MAX_CELLS || p_data->cells[cell] == 0) {
        spdlog::warn("Call made to get_cluster with invalid cell {}, returning CLUSTER_NONE", (int)cell);
        return const_cast<Cluster*>(&CLUSTER_NONE);
    }

    auto it = std::find_if(DSU::cl.begin(), DSU::cl.end(), [c = cell](const auto& cluster) {
        return cluster.members.size() > 1 && std::find(begin(cluster.members), end(cluster.members), c) != end(cluster.members);
    });

    if (it != DSU::cl.end()) {
        return &(*it);
    } else {
        spdlog::debug("Returning CLUSTER_NONE from get_cluster!\nState was \n{}\n", State_Action{*this});
        return const_cast<Cluster*>(&CLUSTER_NONE);
    }
}

// Remove the cells in the chosen cluster and adjusts the colors counter.
void State::kill_cluster(Cluster* cluster)
{
    if (cluster == nullptr)
    {
        auto logger = spdlog::get("m_logger");
        //logger->set_level(spdlog::level::trace);
        spdlog::critical("kill_cluster called with nullptr. Current state is {}", State_Action{*this});
        return;
    } else if (cluster == &CLUSTER_NONE)
    {
        spdlog::warn("kill_cluster called with CLUSTER_NONE. Current state is {}", *this);
        return;
    }

    //assert(cluster != nullptr);
    //assert(cluster->members.size() > 1);    // NOTE: if cluster is 0 initialized this doesn't check anything

    for (auto& cell : cluster->members) {
        Color& color = p_data->cells[cell];
        --colors[color];
        color = COLOR_NONE;
    }
}

void State::apply_action(const ClusterData& cd, StateData& sd)
{
    sd.previous = p_data;                    // Provide info of current state.
    sd.cells = cells();                      // Copy the current grid on the provided StateData object
    p_data = &sd;


    //generate_clusters();    // NOTE: There is a call to generate_clusters at start of get_cluster
    auto* p_cluster = get_cluster(cd.rep);

    if (p_cluster == nullptr)
        spdlog::critical("Call to State::apply_action made with cluster {}", cd);
    if (p_cluster == &CLUSTER_NONE)
        spdlog::warn("Call to State::apply_action made with cluster {}", cd);

    kill_cluster(p_cluster);     // The color counter is decremented here
    pull_cells_down();
    pull_cells_left();

    generate_clusters();
    p_data->key = generate_key();
}


// Need a ClusterData object if undoing multiple times in a row!!!!
void State::undo_action(Action action)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    auto cluster_cells = get_cluster(action)->members;
    Color color = p_data->cells[cluster_cells.back()];
    colors[color] += cluster_cells.size();
}

void State::undo_action(const ClusterData& cd)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    colors[cd.color] += cd.size;
}


ClusterData State::kill_cluster_blind(Cell cell, Color color = COLOR_NONE)
{
    if (color == COLOR_NONE) {
        color = p_data->cells[cell];
        assert(color != COLOR_NONE);
    }

    std::deque<Action> queue{ cell };

    uint8_t cnt = 0;
    Grid& grid = p_data->cells;
    Cell cur;
    while (!queue.empty())
    {
        cur = queue.back();
        queue.pop_back();
        // remove boundary cells of same color as action
        // Look right
        if (cur % WIDTH < WIDTH - 1 && grid[cur+1] == color) {
            grid[cur+1] = COLOR_NONE;
            queue.push_back(cur+1);
            ++cnt;
        }
        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && grid[cur + WIDTH] == color) {
            grid[cur + WIDTH] = COLOR_NONE;
            queue.push_back(cur+WIDTH);
            ++cnt;
        }
        // Look left
        if (cur % WIDTH > 0 && grid[cur-1] == color) {
            grid[cur - 1] = COLOR_NONE;
            queue.push_back(cur-1);
            ++cnt;
        }
        // Look up
        if (cur > WIDTH - 1 && grid[cur - WIDTH] == color) {
            grid[cur - WIDTH] = COLOR_NONE;
            queue.push_back(cur - WIDTH);
            ++cnt;
        }
    }

    if (cnt == 1) {
        grid[cell] = color;
    }

    return {cell, color, cnt};
}

namespace Random {

    enum class Dir {
        UP = -15, LEFT = -1, RIGHT = 1, DOWN = 15, NONE
    };

    Dir pick_dir(int i)
    {
        switch (i)
        {
            case 0:
                return Dir::RIGHT;
            case 1:
                return Dir::LEFT;
            case 2:
                return Dir::UP;
            case 3:
                return Dir::DOWN;
            default:
                return Dir::NONE;
        }
    }

    std::random_device rd;
    std::mt19937 e{rd()}; // or std::default_random_engine e{rd()};
    std::uniform_int_distribution<int> dist{0, 225};

    std::pair<uint8_t, Dir> random_directed_row()
    {
        return std::make_pair(dist(rd) % 15, pick_dir(dist(rd) % 2));
    }

    // Return a random choice, or a NULL cd
    ClusterData choose(const ActionDVec& choices)
    {
        if (choices.empty())
            return ClusterData { CELL_NONE, COLOR_NONE, 0 };
        return choices[dist(rd) % choices.size()];
    }
}

// Pick a random start point and direction and start looking for a cluster.
// NOTE: ******* DO NOT ******* call with terminal state! (inifinite loop)
ClusterData State::apply_random_action()
{
    ClusterData ret { CELL_NONE, COLOR_NONE, 0 };

    while (ret.size < 2)
    {
        auto [row, dir] = Random::random_directed_row();

        Cell cell = row * 15 + int(dir) * 14;       // start at the beggining of the random row if dir is RIGHT, at the end if it's LEFT
        Color color = p_data->cells[cell];
        int steps = 0;

        while (color == COLOR_NONE && steps < 15)   // start walking
        {
            cell += Cell(dir);
            ++steps;
        }

        if (steps < 15) {
            ret = kill_cluster_blind(cell, color);   // try and kill a cluster if you meet one, otherwise repeat.
        }
    }

    return ret;
}


// NOTE: An apply_action method without using cluster information
// void State::apply_action_blind(Action action, StateData& sd)
// {
//     //using pii = std::pair<uint8_t, uint8_t>;

//     sd.previous = p_data;                    // Provide info of current state.
//     sd.cells = cells();                      // Copy the current grid on the provided StateData object
//     p_data = &sd;

//     Color color = sd.cells[action];
//     int cnt = 1;
//     sd.cells[action] = COLOR_NONE;

//     // Manual 'kill_cluster method'
//     // TODO refactor into its own method, have the state manage which kill_cluster
//     // method to call depending on if the r_cluster member is up to date.
//     std::deque<Action> queue{ action };

//     while (!queue.empty())
//     {
//         Cell cur = queue.back();
//         queue.pop_back();
//         // remove boundary cells of same color as action
//         // Look right
//         if (cur % WIDTH < WIDTH - 1 && sd.cells[cur+1] == color) {
//             sd.cells[cur+1] = COLOR_NONE;
//             queue.push_back(cur+1);
//             ++cnt;
//         }
//         // Look down
//         if (cur < (HEIGHT - 1) * WIDTH && sd.cells[cur + WIDTH] == color) {
//             sd.cells[cur + WIDTH] = COLOR_NONE;
//             queue.push_back(cur+WIDTH);
//             ++cnt;
//         }
//         // Look left
//         if (cur % WIDTH > 0 && sd.cells[cur-1] == color) {
//             sd.cells[cur - 1] = COLOR_NONE;
//             queue.push_back(cur-1);
//             ++cnt;
//         }
//         // Look up
//         if (cur > WIDTH - 1 && sd.cells[cur - WIDTH] == color) {
//             sd.cells[cur - WIDTH] = COLOR_NONE;
//             queue.push_back(cur - WIDTH);
//             ++cnt;
//         }
//     }
//
//     colors[color] -= cnt;
//     pull_cells_down();
//     pull_cells_left();
//     generate_clusters();
//     p_data->key = generate_key();
// }


//*************************** DEBUG *************************/

using namespace std;

namespace display {

enum class Color_codes : int {
    BLACK     = 30,
    RED       = 31,
    GREEN     = 32,
    YELLOW    = 33,
    BLUE      = 34,
    MAGENTA   = 35,
    CYAN      = 36,
    WHITE     = 37,
    B_BLACK   = 90,
    B_RED     = 91,
    B_GREEN   = 92,
    B_YELLOW  = 93,
    B_BLUE    = 94,
    B_MAGENTA = 95,
    B_CYAN    = 96,
    B_WHITE   = 97,
};

enum class Shape : int {
    SQUARE,
    DIAMOND,
    B_DIAMOND,
    NONE
};

map<Shape, string> Shape_codes {
    {Shape::SQUARE,    "\u25A0"},
    {Shape::DIAMOND,   "\u25C6"},
    {Shape::B_DIAMOND, "\u25C7"},
};

string shape_unicode(Shape s)
{
    return Shape_codes[s];
}

string color_unicode(Color c)
{
    return std::to_string((int)Color_codes(c + 90));
}

string print_cell(const sg::Grid& grid, Cell ndx, Output output_mode, Cell action_rep = MAX_CELLS, const vector<Cell>& cluster_vec = vector<Cell>())
{
    bool ndx_in_cluster = find(cluster_vec.begin(), cluster_vec.end(), ndx) != cluster_vec.end();
    stringstream ss;

    if (output_mode == Output::CONSOLE)
    {
        Shape shape = ndx == action_rep ? Shape::B_DIAMOND :
            ndx_in_cluster ? Shape::DIAMOND : Shape::SQUARE;

        ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape) << "\033[0m";

        return ss.str();
    }
    else
    {
        // Underline the cluster representative and output the cluster in bold
        string formatter = "";
        if (ndx_in_cluster)
            formatter += "\e[1m]";
        if (ndx == action_rep)
            formatter += "\033[4m]";

        ss << formatter << std::to_string(grid[ndx]) << "\033[0m\e[0m";

        return ss.str();
    }
}

int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

} // namespace display

//****************************** PRINTING BOARDS ************************/


string State_Action::display(sg::Output output_mode) const
{
    bool labels = true;

    const auto& grid = state.p_data->cells;

    std::vector<Cell> cluster_vec;
    if (p_cluster != nullptr) {
        for (auto c : p_cluster->members) {
            cluster_vec.push_back(c);
        }
    }

    stringstream ss {"\n"};

    // For every row
    for (int y = 0; y < HEIGHT; ++y) {
        if (labels) {
            ss << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }
        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x) {
            ss << display::print_cell(grid, Cell(x + y * WIDTH), output_mode, actionrep, cluster_vec) << ' ';
        }
        // Print last entry,
        ss << display::print_cell(grid, Cell(WIDTH - 1 + y * WIDTH), output_mode, actionrep, cluster_vec) << '\n';
    }

    if (labels) {
        ss << std::string(34, '_') << '\n'
             << std::string(5, ' ');

        for (int x : display::x_labels) {

            ss << x << ((x < 10) ? " " : "");
        }
        ss << '\n';
    }

    return ss.str();
}

ostream& operator<<(ostream& _out, const ClusterData& _cd)
{
    return _out << std::to_string(_cd.rep) << std::to_string(_cd.color) << (int)_cd.size;
}

ostream& operator<<(ostream& _out, const State_Action& state_action)
{
    return _out << state_action.display(Output::CONSOLE);

}

ostream& operator<<(ostream& _out, const State& state)
{
    return _out << State_Action({state}).display(Output::CONSOLE);
}

} //namespace sg
