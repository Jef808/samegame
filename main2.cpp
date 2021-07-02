#include "samegame.h"
#include "mcts_impl2/mcts.h"
#include "mcts_impl2/policies.h"

#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <iostream>

#include "display.h"

using namespace sg;
using namespace mcts_impl2;
using namespace sg::display;
using namespace std::literals::chrono_literals;


/**
 * Remove the contribution from the log term after a given number of visits.
 */
template < int N >
struct TimeCutoff_UCB_Func {
    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
        {
            double ret = (n_parent_visits < N ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0)) : 0.000000001);
            return ret + edge.avg_val;
        };
    }
};

/**
 * Weight the colors by the following rule:
 *
 * weight[i] = 1 / (proportion of cells of color i amongst the remaining cells)
 */
struct ColorWeighted_UCB_Func {

    ColorWeighted_UCB_Func(const sg::State& _state)
        : r_state(_state)
    { }

    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        update_weights();
        return [&, expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
        {
            // auto avg_val = edge.avg_val;
            // auto weighted_avg_val = m_weights[color] * avg_val;
            // auto log_term = expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0));
            // auto ret = weighted_avg_val + log_term;
            // PRINT("  avg_val       weighted_avg_val         log_term                 ucb_val\n");
            // PRINT(" ", edge.avg_val, "              ", weighted_avg_val, "              ", log_term, "           ", ret);
            // PRINT("Input a character to continue....");
            // char t; std::cin >> t; std::cin.ignore();
            auto color = std::underlying_type_t<sg::Color>(edge.action.color);
            return m_weights[color] * edge.avg_val + expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0));
        };
    }

    void update_weights()
    {
        double total = std::accumulate(r_state.color_counter().begin(),
                                       r_state.color_counter().end(),
                                       0.0);
        for (int i=0; i<5; ++i)
        {
            m_weights[i] = total / (5.0 * r_state.color_counter()[i]);
        }
    }
    policies::Default_UCB_Func UCB_Func { };
    std::array<double, 5> m_weights;
    /** Don't store the counter itself since the state's address isn't fixed.  */
    const State& r_state;
};

/** Vizualize an action sequence in the console. */
void vizualize(const State& init_state, const auto& actions, int delay_in_ms = 150){
    init_state.view_action_sequence(actions, delay_in_ms);
}


auto run_test(const State& state, int n_iterations, double expl_cst)
{
    // Make a copy of the state to run the algorithm.
    State _state(state);

    // Initialize the Mcts agent with our UCB function.
    using MctsAgent = Mcts<sg::State,
                           sg::ClusterData,
                           //policies::Default_UCB_Func,
                           //ColorWeighted_UCB_Func,
                           TimeCutoff_UCB_Func<30>,
                           128>;
    //MctsAgent mcts(state, ColorWeighted_UCB_Func(state));
    //MctsAgent mcts(state, policies::Default_UCB_Func{});
    MctsAgent mcts(_state, TimeCutoff_UCB_Func<30>{});
    // Configure it.
    mcts.set_exploration_constant(expl_cst);
    mcts.set_max_iterations(n_iterations);
    mcts.set_backpropagation_strategy(MctsAgent::BackpropagationStrategy::best_value);

    // Get the resulting action sequence.
    std::vector<ClusterData> action_seq = mcts.best_action_sequence(MctsAgent::ActionSelection::by_n_visits);
    return action_seq;
}


int main()
{
    std::ifstream _if;
    _if.open("../data/input.txt", std::ios::in);
    if (!_if)
    {
        PRINT("Could not open input file");
        return EXIT_FAILURE;
    }
    // Read the initial data into the state.
    State state(_if);
    _if.close();

    for (int i=0; i<10; ++i)
    {
        auto action_seq = run_test(state, 20000, 1.0);

        // Vizualize it.
        vizualize(state, action_seq);
    }

    return EXIT_SUCCESS;
}
