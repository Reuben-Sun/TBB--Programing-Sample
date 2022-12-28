#include <iostream>
#include <vector>
#include <tbb/tbb.h>

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

int main(){
    std::vector<int> a = {1,4,5,8,9,3,4,6,0};
    std::cout << pmax(a);
    return 0;
}