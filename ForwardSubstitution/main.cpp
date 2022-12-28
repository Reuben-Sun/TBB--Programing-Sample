#include <iostream>
#include <vector>
#include <tbb/tbb.h>

//串行
void normalFS(std::vector<double>& x, const std::vector<double>& a, std::vector<double>& b) {
    const int N = x.size();
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < i; ++j) {
            b[i] -= a[j + i*N] * x[j];
        }
        x[i] = b[i] / a[i + i*N];
    }
}

static std::vector<double> initForwardSubstitution(std::vector<double>& x,
                                                   std::vector<double>& a,
                                                   std::vector<double>& b) {
    const int N = x.size();
    for (int i = 0; i < N; ++i) {
        x[i] = 0;
        b[i] = i*i;
        for (int j = 0; j <= i; ++j) {
            a[j + i*N] = 1 + j*i;
        }
    }

    std::vector<double> b_tmp = b;
    std::vector<double> x_gold = x;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < i; ++j) {
            b_tmp[i] -= a[j + i*N] * x_gold[j];
        }
        x_gold[i] = b_tmp[i] / a[i + i*N];
    }
    return x_gold;
}

int main() {
    const int N = 10;

    std::vector<double> a(N*N);
    std::vector<double> b(N);
    std::vector<double> x(N);

    auto x_gold = initForwardSubstitution(x,a,b);

    /*for(int i = 0; i < N; ++i){
        for(int j = 0; j < N; ++j){
            std::cout << a[i*N+j] << " ";
        }
        std::cout << std::endl;
    }*/
    double serial_time = 0.0;
    {
        tbb::tick_count t0 = tbb::tick_count::now();
        normalFS(x,a,b);
        serial_time = (tbb::tick_count::now() - t0).seconds();
    }
    if ( x_gold != x ) {
        std::cerr << "Error: results do not match gold version" << std::endl;
    }
    std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
    return 0;
}

