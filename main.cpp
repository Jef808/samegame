// main.cpp

#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <memory>
#include <sstream>
#include <spdlog/spdlog.h>
//#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>
//#include "graphlog.h"
#include "samegame.h"
#include "mcts.h"
//#include "debug.h"


using namespace std;
using namespace literals::chrono_literals;

using namespace mcts;


using TimePoint = std::chrono::milliseconds::rep;

//std::chrono::time_point<std::chrono::steady_clock> search_start;

auto init_time()
{
    return std::chrono::steady_clock::now();
}

TimePoint time_elapsed(auto tik)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now() - tik).count();
}

// void iterate_with_timeout(Agent& agent, Node* node, const std::shared_ptr<spdlog::logger>& logger)
// {
//     auto iterate = [&]() {
//         Reward reward = agent.rollout_policy(node);
//         logger->trace("Top 10 children of the new node:\n");
//         auto children = agent.current_node()->children;
//         for (int i=0; i<10; ++i) {
//             if (children[i].cd.rep == sg::MAX_CELLS)
//                 break;
//             logger->trace("Child {}: \n{}\nReward = {}\n", i, sg::State_Action{agent.state, children[i].cd.rep, agent.state.get_cluster(children[i].cd.rep)}, children[i].value_assured);
//         }

//         agent.state.generate_clusters();

//         agent.backpropagate(node, reward);
//     };

//     std::packaged_task<void()> task(iterate);
//     auto future = task.get_future();
//     std::thread thr(std::move(task));

//     if (future.wait_for(2s) != std::future_status::timeout)
//     {
//         thr.join();
//         future.get(); // this will propagate exception from f() if any
//     }
//     else
//     {
//         thr.detach(); // we leave the thread still running
//         spdlog::apply_all([&] (std::shared_ptr<spdlog::logger> l) { l->flush(); });

//         spdlog::debug("Timeout!");
//         spdlog::error("Timeout");
//         throw std::runtime_error("Timeout");
//     }
// }

void init_loggers()
{
    try
    {
        auto logger = spdlog::rotating_logger_mt("m_logger", "logs/logfile.txt", 1048576 * 5, 3, true);
        //spdlog::set_level(spdlog::level::critical);
        logger->set_level(spdlog::level::trace);
        auto dotviz = spdlog::basic_logger_mt("dotviz", "logs/tree.txt");
        //dotviz->set_level(spdlog::level::info);

        logger->set_level(spdlog::level::trace);
        spdlog::set_level(spdlog::level::off);
        dotviz->set_level(spdlog::level::off);

    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::printf("Failed to initialize loggers: %s\n", ex.what());
        return;
    }
}

int main(int args, char* argv[])
{
    init_loggers();

    auto logger = spdlog::get("m_logger");
    auto console = spdlog::default_logger();
    auto graph = spdlog::get("dotviz");

    spdlog::debug("test debug");

    graph->set_pattern("%v");

    console->set_level(spdlog::level::debug);
    logger->set_level(spdlog::level::trace);

    logger->trace("Test_trace");
    logger->debug("test_debug");


    //spdlog::set_level(spdlog::level::debug);
    // auto logger = spdlog::get("m_logger");
    // logger->set_level(spdlog::level::trace);

    //("m_logger", "logs/logfile.txt", 1048576 * 5, 3);
    // console->set_level(spdlog::level::debug);
    //logger->set_level(spdlog::level::trace);

    sg::State::init();

    sg::StateData sd_root = {};

    ifstream _if;
    _if.open("../data/input.txt", ios::in);
    sg::State state (_if, &sd_root);
    _if.close();

    //spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->info("Initial State : \n\n{}\n", state); });

    // Debug::set_debug_init_children(true);
    // Debug::set_debug_counters(true);
    // Debug::set_debug_final_scores(true);
    // Debug::set_debug_tree(true);
    // Debug::set_debug_main_methods(true);

    Agent agent(state);
    agent.create_root();

    logger->trace("Initialized root with key {}:\n", (int)agent.root->key);

    //spdlog::debug("Call to MCTS algorithm");

    // for (int i=0; i<agent.root->n_children; ++i)
    // {
    //     auto child = agent.root->children[i];
    //     logger->trace("rep = {}, simul value = {}", child.cd.rep, child.value_assured);
    // }

    graph->info("digraph T {\nnode [shape=record];\n");
    graph->info("{} [label=\"n={}\"];\n", 0, agent.root->n_visits);
    spdlog::debug("added root to the graph.");

    int graph_cnt = 1;
    std::vector<std::pair<Edge*, Node*>> GraphIndex;
    GraphIndex.push_back({nullptr, agent.root});

    for (int k=0; k<30; ++k)
    {
        assert(agent.is_root(agent.current_node()));

        agent.state.generate_clusters();

        assert(agent.is_root(agent.current_node()));

        auto n_ch = agent.current_node()->n_children;
        auto children = agent.current_node()->children;

        std::sort(begin(children), begin(children) + n_ch, [](const auto& a, const auto& b) {
            return a.value_assured > b.value_assured;
        });

        // logger->trace("Top 10 children of root:\n", state, n_ch);
        // for (int i=0; i<10; ++i) {
        //     if (children[i].cd.rep == sg::MAX_CELLS)
        //         break;
        //     logger->trace("Child {}: \n{}{}\n", i, sg::State_Action{state, children[i].cd.rep, state.get_cluster(children[i].cd.rep)}, children[i]);
        // }

        Node* node = agent.tree_policy();
        Node* p_parent;
        Edge* p_edge;
        Node* p_node_cpy;

        //std::shared_ptr<Node> node_sptr;
        //std::shared_ptr<Edge> edge_sptr;
        //std::shared_ptr<Node> parent_sptr;


        bool should_add = false;//node->n_visits == 0;


        if (should_add) {

            spdlog::debug("Found a new node with key {}", (int)node->key);

            p_node_cpy = node;
            p_parent = agent.nodes[agent.get_ply()-1];
            p_edge = agent.actions[agent.get_ply()-1];

            assert(p_parent != nullptr && p_edge != nullptr);

            if (*p_node_cpy->parent != *p_edge)
            {
                spdlog::debug("***** Need to sync agent.actions[ply] with agent.nodes[ply+1]");
            }
            //node_sptr = std::make_shared<Node>(node);

            //edge_sptr = std::make_shared<Edge>(agent.actions[agent.get_ply() - 1]);
            //parent_sptr = std::make_shared<Node>(agent.nodes[agent.get_ply() - 1]);


            GraphIndex.push_back(std::make_pair(p_edge, p_node_cpy));
        }

        // auto saved_ply = agent.get_ply();
        // while (!agent.is_root(agent.current_node()))
        // {
        //     agent.undo_action();
        // }

        // if (agent.get_ply() != 1) {
        //     spdlog::error("Did not go down to the root!");
        //     throw std::runtime_error("Did not go down to the root!");
        // }

        // logger->trace("Descent # {} resulted in the following path:\n", agent.descent_cnt);
        // while (agent.get_ply() < saved_ply)
        // {
        //     auto ad = agent.actions[agent.get_ply()];
        //     logger->trace("\n{}\nReward = {}\n", sg::State_Action{state, ad->cd.rep, state.get_cluster(ad->cd.rep)}, ad->value_sg_from_root);
        //     agent.apply_action(agent.actions[agent.get_ply()]->cd);
        // }

        // if (saved_ply != agent.get_ply()) {
        //     spdlog::error("Saved ply was not restored after displaying path!");
        //     throw std::runtime_error("Did not go back to saved ply!");
        // }

        //logger->trace("Rollout: out on node \n{}\n", );


        Reward reward = agent.rollout_policy(node);

        // if (agent.get_ply() > 1) {
        //     spdlog::debug("Rollout gave a reward of {}, the path before backpropagating is\n", reward);
        //     for (int i=1; i<agent.get_ply(); ++i)
        //     {
        //         spdlog::debug("Root: n={}\n", agent.root->n_visits);
        //         spdlog::debug("Action {}: n={}(node n={}),av={}\n", agent.actions[i]->cd.rep, agent.actions[i]->n_visits, agent.nodes[i]->n_visits, agent.actions[i]->reward_avg_visit);
        //     }
        // }

        agent.backpropagate(node, reward);

        if (agent.iteration_cnt > 0 && agent.iteration_cnt % 100 == 0)
        {
            logger->trace("After {} iterations, the Root's children are\n", agent.iteration_cnt);
            for (const auto& edge : agent.root->children)
            {
                logger->trace("{}\n", edge);
            }

        }
        // if (agent.get_ply() > 1) {
        //     spdlog::debug("After backpropagating:\n");
        //     for (int i=1; i<agent.get_ply()+1; ++i)
        //     {
        //         spdlog::debug("Root: n={}\n", agent.root->n_visits);
        //         spdlog::debug("Action {}: n={}(node n={}),av={}\n", agent.actions[i]->cd.rep, agent.actions[i]->n_visits, agent.nodes[i]->n_visits, agent.actions[i]->reward_avg_visit);
        //     }
        // }






        if (should_add) {
            auto [p_edge, p_node] = GraphIndex.back();

            assert(p_edge != nullptr && p_node != nullptr);

            auto n_vis = p_node->n_visits;
            auto val = p_edge->value_assured;
            auto avg = p_edge->reward_avg_visit;

            graph->info("{} [label=\"n={}\nv={:.1f},av={:.1f}\"];\n", graph_cnt, n_vis, val/agent.value_global_max, avg);
            spdlog::debug("Just added node #{} to the graph", graph_cnt);

            int num_key_found = 0;

            for (int i=0; i < graph_cnt; ++i) {
                if (p_parent->key == GraphIndex[i].second->key) {
                    graph->info("{} -> {} ;\n", i, graph_cnt);
                    spdlog::debug("Found parent, index is {}", i);
                }
            }
            ++graph_cnt;
        }
        ++agent.iteration_cnt;
    }

    graph->info("}");
    graph->flush();

    //node = agent.tree_policy();

    //reward = agent.rollout_policy(node);
    //auto action = agent.MCTSBestAction();

    //agent.print_final_score();

    //ClusterData cd = agent.MCTSBestAction();

    //agent.print_debug_cnts(cout);
    // string buf;

    //while (!state.is_terminal())
    // {
    //     while (Cell(action) > MAX_CELLS - 1 || state.cells()[Cell(action)] == COLOR_NONE)
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
    spdlog::info("Game over!");

    return 0;
}
