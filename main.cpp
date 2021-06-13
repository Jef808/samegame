// main.cpp

#include <iostream>
#include <fstream>
#include <thread>
#include <iostream>
#include <fstream>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/stopwatch.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "samegame.h"
#include "mcts.h"
#include "types.h"


using namespace std;
using namespace mcts;
using namespace sg;
//using namespace literals::chrono_literals;

void output_pv(Agent& agent)
{
    auto logger = spdlog::get("m_logger");

    assert(agent.current_node() == agent.root);

    array<mcts::SearchData, MAX_DEPTH> pv { };
    int ply = 1;

    Node* node = agent.root;
    Edge* edge;
    pv[ply].r = 0;

    //state.generate_clusters();
    logger->trace("Final path is:\n");
    while (node->n_visits > 1 && node->n_children > 0)
    {
        edge = agent.best_visits(node);

        if (edge->cd.size < 2) {
            spdlog::debug("Terminal state or invalid edge??");
            break;
        }

        pv[ply+1].r = pv[ply].r + agent.evaluate_valid_action(edge->cd);

        State_Action sa {agent.state, edge->cd.rep};

        logger->trace("   *** Move {} rep = {} score = {}\n", ply, edge->cd.rep, pv[ply].r);
        logger->trace("\n{}\n", sa);

        agent.apply_action(edge->cd);
        ++ply;
        //state.generate_clusters();    // Generated during the call to apply_action to generate the key
    }

    logger->trace("*********** pv over.\n\n    Final Score : {}\n\n\n", pv[ply].r);
}

ClusterData one_move(Agent& agent, State& state, int iters = MAX_ITER, double exploration_cst = EXPLORATION_CST)
{
    auto logger = spdlog::get("m_logger");

    agent.reset();
    agent.set_max_iterations(iters);
    agent.set_exploration_constant(exploration_cst);

    agent.set_root();

    spdlog::stopwatch sw;
    auto action = agent.MCTSBestAction();
    logger->trace("\n    \033[32m::one_move:: {:.2} seconds\033[m]", sw);
    return action;
}

void changing_roots(State& state)
{
    auto logger = spdlog::get("m_logger");
    Agent agent(state);
    agent.set_max_iterations(1000);

    int score = 0;

    // NOTE: Calling set_root inside of MCTSBestAction! If called twice in a row, it messes up the state!!

    logger->trace("Starting search with 1000 iterations per move");
    spdlog::stopwatch sw_glob;
    while (!state.is_terminal())
    {
        ClusterData action = agent.MCTSBestAction();

        if (action.rep == CELL_NONE)
            break;

        score += (action.size - 2) * (action.size - 2);

        spdlog::info("\n{}\n", state);
        agent.apply_action(action);
        this_thread::sleep_for(500ms);
        spdlog::info("\n{}\n", State_Action{ state, action.rep });

    }

    logger->trace("\n{}\n\nGame Over! Total score is {}, total time taken {}", state, score, sw_glob);
}

int main()
{
    State::init();
    auto logger = spdlog::rotating_logger_mt("m_logger", "logs/logfile.txt", 1048576 * 5, 3);
    auto console = spdlog::default_logger();

    spdlog::flush_every(std::chrono::seconds(3));

    console->set_level(spdlog::level::debug);
    logger->set_level(spdlog::level::trace);
    //auto graph = spdlog::get("dotviz");

    sg::StateData sd_root{ };
    ifstream _if;
    _if.open("../data/input.txt", ios::in);
    sg::State state (_if, sd_root);
    _if.close();

    Agent agent(state);


    agent.debug_tree_policy = true;


    int _in = 0;
    double exp_cst = 0.04;

    while (1) {

        spdlog::debug("Exploration Constant is {}\n>>> Enter an increment/decrement (input * 0.005 will be applied)", exp_cst);

        cin >> _in; cin.ignore();
        exp_cst += 0.005 * _in;
        agent.reset();
        agent.set_root();

        spdlog::info("Starting search...");
        ClusterData cd = one_move(agent, state, 400, exp_cst);
        logger->trace("Search with exp_cst=\033[35m{}\033[m found \033[35m{}\033[m\nThe other moves are\n", exp_cst, cd.rep);
        spdlog::debug("Search with exp_cst=\033[35m{}\033[m found \033[35m{}\033[m\nThe other moves are\n", exp_cst, cd.rep);

        std::sort(begin(agent.root->children), end(agent.root->children), [](const auto& a, const auto& b) {
                return a.reward_avg_visit > b.reward_avg_visit;
            });

        for (const auto& c : agent.root->children)
        {
            if (c.cd.rep == cd.rep) {
                logger->trace("    \033[35m{}\033[m", c);
                spdlog::debug("    \033[35m{}\033[m", c);
            } else {
                logger->trace("    {}", c);
                spdlog::debug("    {}", c);
            }
        }

        logger->trace("{}", agent);
    }
}




// void test_ucb_functions(Agent& agent, State& state, int iters)
// {

//     auto logger = spdlog::get("m_logger");

//     logger->trace("*************************************\n\n     Testing ucb for {} iterations\n\n***********************************************\n\n", iters);

//     agent.set_root();

//     // for (int i=0; i<agent.root->n_children; ++i)
//     // {
//     //     auto child = agent.root->children[i];
//     //     logger->trace("rep = {}, simul value = {}", child.cd.rep, child.val_best);
//     // }

//     // graph->info("digraph T {\nnode [shape=record];\n");
//     // graph->info("{} [label=\"n={}\"];\n", 0, agent.root->n_visits);
//     // spdlog::debug("added root to the graph.");

//     //int graph_cnt = 1;
//     //std::vector<std::pair<Edge*, Node*>> GraphIndex;
//     //GraphIndex.push_back({nullptr, agent.root});
//     //logger->trace("Starting the for loop");

//     spdlog::stopwatch sw;

//     // Averages the rollout rewards over 500 consecutive tries. See if it improves.

//     double cur_avg = 0;
//     int cnt = 0;
//     int n_avg = 0;

//     double cur_avg_ucb = 0;
//     vector<double> root_ucb_values (agent.root->n_children, 0.0);
//     vector<double> root_first_terms (agent.root->n_children, 0.0);

//     vector<Edge*> root_children;

//     for (int k=0; k<iters; ++k)
//     {
//         assert(agent.is_root(agent.current_node()));

//         Node* node = agent.tree_policy();
//         Reward reward = agent.rollout_policy(node);

//         if (k == 2)
//         {
//             for (int i=0; i<agent.root->n_children; ++i)
//             {
//                 root_children.push_back(&agent.root->children[i]);
//             }
//         }
//         ++cnt;
//         cur_avg += (reward - cur_avg) / cnt;

//         agent.backpropagate(node, reward);

//         if (k > 2)
//         {
//             Edge* e;
//             for (int i=0; i<agent.root->n_children; ++i)
//             {
//                 e = root_children[i];

//                 Reward res = e->n_visits > 0 ? e->reward_avg_visit : e->val_best;
//                 auto first_term = agent.value_to_reward(res);
//                 auto ucb_val = agent.ucb(agent.root, *e);

//                 root_ucb_values[i] += (ucb_val - root_ucb_values[i]) / cnt;
//                 root_first_terms[i] += (first_term - root_first_terms[i]) / cnt;
//             }
//         }

//         ++agent.cnt_iterations;

//         if (cnt == 100) {
//             logger->trace("average rewards (over 100 samples) # {}: avg_value is {}\nUCBS:\n", k, cur_avg);
//             spdlog::info("average rewards (over 100 samples) # {}: avg_value is {}\nUCBS:\n", k, cur_avg);
//             for (int i=0; i<agent.root->n_children; ++i)
//             {
//                 logger->trace("    Child {} avg first term = {}, avg ucb = {}\n", i, root_first_terms[i], root_ucb_values[i]);
//                 spdlog::info("    Child {} avg first term = {}, avg ucb = {}\n", i, root_first_terms[i], root_ucb_values[i]);
//             }
//             logger->flush();
//             logger->trace("Time taken: {} seconds\nAgent's stats: {}\n", sw, agent);
//             logger->trace("global max = {}, global min = {}\n", agent.value_global_max, agent.value_global_min);
//             spdlog::info("global max = {}, global min = {}\n", agent.value_global_max, agent.value_global_min);
//             ++n_avg;
//             cnt = 0;
//             cur_avg = 0;
//             root_first_terms.clear();
//             root_ucb_values.clear();
//         }
//     }
//     //logger->trace("{:.2} seconds for {} iterations", sw, iters);

//     //output_pv(agent);
// }


        // Node* p_parent;
        // Edge* p_edge;
        // Node* p_node_cpy;

        //std::shared_ptr<Node> node_sptr;
        //std::shared_ptr<Edge> edge_sptr;
        //std::shared_ptr<Node> parent_sptr;


        // bool should_add = false;//node->n_visits == 0;


        // if (should_add) {

        //     spdlog::debug("Found a new node with key {}", (int)node->key);

        //     p_node_cpy = node;
        //     p_parent = agent.nodes[agent.get_ply()-1];
        //     p_edge = agent.actions[agent.get_ply()-1];

        //     assert(p_parent != nullptr && p_edge != nullptr);

        //     if (*p_node_cpy->parent != *p_edge)
        //     {
        //         spdlog::debug("***** Need to sync agent.actions[ply] with agent.nodes[ply+1]");
        //     }
        //     //node_sptr = std::make_shared<Node>(node);

        //     //edge_sptr = std::make_shared<Edge>(agent.actions[agent.get_ply() - 1]);
        //     //parent_sptr = std::make_shared<Node>(agent.nodes[agent.get_ply() - 1]);


        //     GraphIndex.push_back(std::make_pair(p_edge, p_node_cpy));
        // }

        // auto saved_ply = agent.get_ply();
        // while (!agent.is_root(agent.current_node()))
        // {
        //     agent.undo_action();
        // }

        // if (agent.get_ply() != 1) {
        //     spdlog::error("Did not go down to the root!");
        //     throw std::runtime_error("Did not go down to the root!");
        // }

        // logger->trace("Descent # {} resulted in the following path:\n", agent.cnt_descent);
        // while (agent.get_ply() < saved_ply)
        // {
        //     auto ad = agent.actions[agent.get_ply()];
        //     logger->trace("\n{}\nReward = {}\n", sg::State_Action{state, ad->cd.rep, state.get_cluster(ad->cd.rep)}, ad->evalute_action_from_root);
        //     agent.apply_action(agent.actions[agent.get_ply()]->cd);
        // }

        // if (saved_ply != agent.get_ply()) {
        //     spdlog::error("Saved ply was not restored after displaying path!");
        //     throw std::runtime_error("Did not go back to saved ply!");
        // }

        //logger->trace("Rollout: out on node \n{}\n", );




        // if (agent.get_ply() > 1) {
        //     spdlog::debug("Rollout gave a reward of {}, the path before backpropagating is\n", reward);
        //     for (int i=1; i<agent.get_ply(); ++i)
        //     {
        //         spdlog::debug("Root: n={}\n", agent.root->n_visits);
        //         spdlog::debug("Action {}: n={}(node n={}),av={}\n", agent.actions[i]->cd.rep, agent.actions[i]->n_visits, agent.nodes[i]->n_visits, agent.actions[i]->reward_avg_visit);
        //     }
        // }


        // if (agent.cnt_iterations > 0 && agent.cnt_iterations % 100 == 0)
        // {
        //     logger->trace("After {} iterations, the Root's children are\n", agent.cnt_iterations);
        //     for (const auto& edge : agent.root->children)
        //     {
        //         logger->trace("{}\n", edge);
        //     }

        // }
        // if (agent.get_ply() > 1) {
        //     spdlog::debug("After backpropagating:\n");
        //     for (int i=1; i<agent.get_ply()+1; ++i)
        //     {
        //         spdlog::debug("Root: n={}\n", agent.root->n_visits);
        //         spdlog::debug("Action {}: n={}(node n={}),av={}\n", agent.actions[i]->cd.rep, agent.actions[i]->n_visits, agent.nodes[i]->n_visits, agent.actions[i]->reward_avg_visit);
        //     }
        // }






        // if (should_add) {
        //     auto [p_edge, p_node] = GraphIndex.back();

        //     assert(p_edge != nullptr && p_node != nullptr);

        //     auto n_vis = p_node->n_visits;
        //     auto val = p_edge->val_best;
        //     auto avg = p_edge->reward_avg_visit;

        //     graph->info("{} [label=\"n={}\nv={:.1f},av={:.1f}\"];\n", graph_cnt, n_vis, val/agent.value_global_max, avg);
        //     spdlog::debug("Just added node #{} to the graph", graph_cnt);

        //     int num_key_found = 0;

        //     for (int i=0; i < graph_cnt; ++i) {
        //         if (p_parent->key == GraphIndex[i].second->key) {
        //             graph->info("{} -> {} ;\n", i, graph_cnt);
        //             spdlog::debug("Found parent, index is {}", i);
        //         }
        //     }
        //     ++graph_cnt;
        // }



    // graph->info("}");
    // graph->flush();

    //node = agent.tree_policy();

    //reward = agent.rollout_policy(node);
    //auto action = agent.MCTSBestAction();

    //agent.print_final_score();

    //ClusterData cd = agent.MCTSBestAction();

    //agent.print_debug_cnts(cout);
    // string buf;

    //while (!state.is_terminal())
    // {
    //     while (Cell(action) > MAX_CELLS - 1 || state.cells()[Cell(action)] == Color::Empty)
    //     {
    //         stringstream ss;
    //         int x, y;
    //         buf.clear();

    //         cout << "Choose a cell (row column) to play a move, or U to undo a move.\n" << endl;
    //         Debug::display(cout, state);

    //         getline(cin, buf);

    //         if (buf[0] == 'U') {
    //             state.undo_action(action);
    //         }
    //         else {
    //             ss << buf;
    //             ss >> x >> y ;
    //             action = Action(x + (14 - y) * WIDTH);
    //         }
    //     }

    //     Cluster* cluster = state.get_cluster(Cell(action));
    //     cout << "Chosen cluster: " << endl;
    //     Debug::display(cout, state, Cell(action), cluster);

    //     cout << "Effect of apply_action_blind: " << endl;
    //     agent.apply_action_blind(action);

    //     //this_thread::sleep_for(500ms);

    //     //agent.apply_action(action);

    //     action = ACTION_NONE;
    // }

    //Debug::display(cout, state);
//}
