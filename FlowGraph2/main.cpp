#include <iostream>
#include <tbb/tbb.h>

void fig_3_3(){
    //创建图对象
    tbb::flow::graph g;
    //创建节点
    tbb::flow::function_node<int, std::string> my_first_node(
            g, tbb::flow::unlimited,
            [](const int &in) -> std::string{
                std::cout << "first node received: " << in << std::endl;
                return std::to_string(in);
            }
        );
    tbb::flow::function_node<std::string> my_second_node(
            g, tbb::flow::unlimited,
            [](const std::string &in){
                std::cout << "second node received: " << in << std::endl;
            }
        );
    //链接
    tbb::flow::make_edge(my_first_node, my_second_node);
    //发送消息
    my_first_node.try_put(10);
    //等待图执行
    g.wait_for_all();
}

void fig_3_5() {
    // step 1: construct the graph
    tbb::flow::graph g;

    // step 2: make the nodes
    tbb::flow::function_node<int, std::string> my_node{g,
                                                       tbb::flow::unlimited,
                                                       []( const int& in ) -> std::string {
                                                           std::cout << "received: " << in << std::endl;
                                                           return std::to_string(in);
                                                       }
    };

    tbb::flow::function_node<int, double> my_other_node{g,
                                                        tbb::flow::unlimited,
                                                        [](const int& in) -> double {
                                                            std::cout << "other received: " << in << std::endl;
                                                            return double(in);
                                                        }
    };

    tbb::flow::join_node<std::tuple<std::string, double>,
            tbb::flow::queueing> my_join_node{g};

    tbb::flow::function_node<std::tuple<std::string, double>,
            int> my_final_node{g,
                               tbb::flow::unlimited,
                               [](const std::tuple<std::string, double>& in) -> int {
                                   std::cout << "final: " << std::get<0>(in)
                                             << " and " << std::get<1>(in) << std::endl;
                                   return 0;
                               }
    };

    // step 3: add the edges
    make_edge(my_node, tbb::flow::input_port<0>(my_join_node));
    make_edge(my_other_node, tbb::flow::input_port<1>(my_join_node));
    make_edge(my_join_node, my_final_node);

    // step 4: send messages
    my_node.try_put(1);
    my_other_node.try_put(2);
    // step 5: wait for the graph to complete
    g.wait_for_all();
}

static void warmupTBB() {
    //tbb::task_scheduler_init::default_num_threads()已经弃用
    tbb::parallel_for(0, tbb::this_task_arena::max_concurrency(), [](int) {
        tbb::tick_count t0 = tbb::tick_count::now();
        while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    });
}

int main() {
    warmupTBB();
    double parallel_time = 0.0;
    {
        tbb::tick_count t0 = tbb::tick_count::now();
        fig_3_5();
        parallel_time = (tbb::tick_count::now() - t0).seconds();
    }

    std::cout << "parallel_time == " << parallel_time << " seconds" << std::endl;
    return 0;
}
