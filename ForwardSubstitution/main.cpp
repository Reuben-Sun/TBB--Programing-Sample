#include <iostream>
#include <vector>
#include <tbb/tbb.h>

//串行
void serialFS(std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b) {
    const int N = x.size();
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < i; ++j) {
            b[i] -= a[j + i * N] * x[j];
        }
        x[i] = b[i] / a[i + i * N];
    }
}

//串型+分块
void serialBlockFS(std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b) {
    const int N = x.size();
    const int block_size = 512;
    const int num_blocks = N / block_size;

    for (int r = 0; r < num_blocks; ++r) {
        for (int c = 0; c <= r; ++c) {
            int i_start = r * block_size, i_end = i_start + block_size;
            int j_start = c * block_size, j_max = j_start + block_size - 1;
            for (int i = i_start; i < i_end; ++i) {
                int j_end = (i <= j_max) ? i : j_max + 1;
                for (int j = j_start; j < j_end; ++j) {
                    b[i] -= a[j + i * N] * x[j];
                }
                if (j_end == i) {
                    x[i] = b[i] / a[i + i * N];
                }
            }
        }
    }
}

//并行
void parallelFS(std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b) {
    const int N = x.size();
    const int block_size = 512;
    const int num_blocks = N / block_size;
    //tbb::atomic已经废弃
    std::vector<std::atomic<char>> ref_count(num_blocks * num_blocks);
    for (int r = 0; r < num_blocks; ++r) {
        for (int c = 0; c <= r; ++c) {
            if (r == 0 && c == 0)
                ref_count[r * num_blocks + c] = 0;
            else if (c == 0 || r == c)
                ref_count[r * num_blocks + c] = 1;
            else
                ref_count[r * num_blocks + c] = 2;
        }
    }

    using BlockIndex = std::pair<size_t, size_t>;
    BlockIndex top_left(0, 0);
    //tbb::parallel_do已经废弃
    tbb::parallel_for_each(&top_left, &top_left + 1,
                           [&](const BlockIndex &bi, tbb::feeder<BlockIndex> &feeder) {
                               size_t r = bi.first;
                               size_t c = bi.second;
                               int i_start = r * block_size, i_end = i_start + block_size;
                               int j_start = c * block_size, j_max = j_start + block_size - 1;
                               for (int i = i_start; i < i_end; ++i) {
                                   int j_end = (i <= j_max) ? i : j_max + 1;
                                   for (int j = j_start; j < j_end; ++j) {
                                       b[i] -= a[j + i * N] * x[j];
                                   }
                                   if (j_end == i) {
                                       x[i] = b[i] / a[i + i * N];
                                   }
                               }
                               // add successor to right if ready
                               if (c + 1 <= r && --ref_count[r * num_blocks + c + 1] == 0) {
                                   feeder.add(BlockIndex(r, c + 1));
                               }
                               // add succesor below if ready
                               if (r + 1 < (size_t) num_blocks && --ref_count[(r + 1) * num_blocks + c] == 0) {
                                   feeder.add(BlockIndex(r + 1, c));
                               }
                           }
    );
}

//独立图
using Node = tbb::flow::continue_node<tbb::flow::continue_msg>;
using NodePtr = std::shared_ptr<Node>;
NodePtr createNode(tbb::flow::graph &g, int r, int c, int block_size,
                   std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b);
void addEdges(std::vector<NodePtr> &nodes, int r, int c, int block_size, int num_blocks);
void dependencyGraphFS(std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b){
    const int N = x.size();
    const int block_size = 512;
    const int num_blocks = N / block_size;

    std::vector<NodePtr> nodes(num_blocks * num_blocks);
    //创建图对象
    tbb::flow::graph g;
    for(int r = num_blocks-1; r >= 0; --r){
        for(int c = r; c >= 0; --c){
            //创建图节点
            nodes[r * num_blocks + c] = createNode(g, r, c, block_size, x, a, b);
            //链接
            addEdges(nodes, r, c, block_size, num_blocks);
        }
    }
    //传入消息
    nodes[0]->try_put(tbb::flow::continue_msg());
    //等待完成
    g.wait_for_all();
}


//初始化
static std::vector<double> initForwardSubstitution(std::vector<double> &x,
                                                   std::vector<double> &a,
                                                   std::vector<double> &b) {
    const int N = x.size();
    for (int i = 0; i < N; ++i) {
        x[i] = 0;
        b[i] = i * i;
        for (int j = 0; j <= i; ++j) {
            a[j + i * N] = 1 + j * i;
        }
    }

    std::vector<double> b_tmp = b;
    std::vector<double> x_gold = x;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < i; ++j) {
            b_tmp[i] -= a[j + i * N] * x_gold[j];
        }
        x_gold[i] = b_tmp[i] / a[i + i * N];
    }
    return x_gold;
}

int main() {
    const int N = 32768;

    std::vector<double> a(N * N);
    std::vector<double> b(N);
    std::vector<double> x(N);

    auto x_gold = initForwardSubstitution(x, a, b);

    /*for(int i = 0; i < N; ++i){
        for(int j = 0; j < N; ++j){
            std::cout << a[i*N+j] << " ";
        }
        std::cout << std::endl;
    }*/
    double serial_time = 0.0;
    {
        tbb::tick_count t0 = tbb::tick_count::now();
//        serialFS(x,a,b);
//        serialBlockFS(x, a, b);
//        parallelFS(x, a, b);
        dependencyGraphFS(x, a, b);
        serial_time = (tbb::tick_count::now() - t0).seconds();
    }
    for (int i = 0; i < N; ++i) {
        if (x[i] > 1.1 * x_gold[i] || x[i] < 0.9 * x_gold[i]) {
            std::cerr << "  at " << i << " " << x[i] << " != " << x_gold[i] << std::endl;
        }
    }
    std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
    return 0;
}

NodePtr createNode(tbb::flow::graph &g, int r, int c, int block_size,
                   std::vector<double> &x, const std::vector<double> &a, std::vector<double> &b){
    const int N = x.size();
    return std::make_shared<Node>(
            g,
            [r, c, block_size, N, &x, &a, &b](const tbb::flow::continue_msg & msg){
                int i_start = r * block_size, i_end = i_start + block_size;
                int j_start = c * block_size, j_max = j_start + block_size -1;
                for(int i = i_start; i < i_end; ++i){
                    int j_end = (i <= j_max) ? i : j_max+1;
                    for(int j = j_start; j < j_end; ++j){
                        b[i] -= a[j + i*N] * x[j];
                    }
                    if(j_end == i){
                        x[i] = b[i] / a[i + i*N];
                    }
                }
                return msg;
            }
        );
}

void addEdges(std::vector<NodePtr> &nodes, int r, int c, int block_size, int num_blocks){
    NodePtr np = nodes[r * num_blocks + c];
    if(c + 1 < num_blocks && r != c){
        tbb::flow::make_edge(*np, *nodes[r * num_blocks + c + 1]);
    }
    if(r + 1 < num_blocks){
        tbb::flow::make_edge(*np, *nodes[(r+1) * num_blocks + c]);
    }
}