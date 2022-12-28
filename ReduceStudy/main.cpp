#include <iostream>
#include <vector>
#include <tbb/tbb.h>

//求最大值
int pmax(const std::vector<int> &arr){
    int max_value = tbb::parallel_reduce(
            tbb::blocked_range<int>(0, arr.size()),
            std::numeric_limits<int>::min(),
            [&](const tbb::blocked_range<int> &r, int init) -> int{
                for(int i = r.begin(); i != r.end(); ++i){
                    init = std::max(init, arr[i]);
                }
                return init;
            },
            [](int x, int y) -> int{
                return std::max(x, y);
            }
            );
    return max_value;
}

//求pi
double calcPI(int degree){
    double dx = 1.0 / degree;
    double sum = tbb::parallel_reduce(
            tbb::blocked_range<int>(0, degree),
            0.0,
            [=](const tbb::blocked_range<int> &r, double init) -> double{
                for(int i = r.begin(); i != r.end(); ++i){
                    double x = (i + 0.5)*dx;
                    double h = std::sqrt(1 - x*x);  //勾股定理
                    init += h * dx;
                }
                return init;
            },
            [](double x, double y) -> double{
                return x+y;
            }
        );
    return 4 * sum;
}

int main(){
    std::vector<int> a = {1,4,5,8,9,3,4,6,0};
    std::cout << pmax(a) << std::endl;
    std::cout << "PI = " << calcPI(100000) << std::endl;
    return 0;
}