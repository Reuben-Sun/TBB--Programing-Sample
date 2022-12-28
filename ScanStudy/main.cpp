#include <iostream>
#include <vector>
#include <tbb/tbb.h>

//串行前缀和
int normalPrefix(const std::vector<int> &v, std::vector<int> &psum){
    int N = v.size();
    psum[0] = v[0];
    for(int i = 1; i < N; ++i){
        psum[i] = psum[i-1] + v[i];
    }
    return psum[N-1];
}

//并行前缀和
int parallelPrefix(const std::vector<int> &v, std::vector<int> &psum){
    int N = v.size();
    psum[0] = v[0];
    int final_sum = tbb::parallel_scan(
            tbb::blocked_range<int>(1, N),
            (int)0,
            [&v, &psum](const tbb::blocked_range<int> &r, int sum, bool is_final_scan) -> int{
                for(int i = r.begin(); i < r.end(); ++i){
                    sum += v[i];
                    if(is_final_scan){
                        psum[i] = sum;
                    }
                }
                return sum;
            },
            [](int x, int y){
                return x+y;
            }
        );
    return final_sum;
}

//薄板可见性
void visibility(const std::vector<double> &heights, std::vector<bool> & visible, double dx){
    const int N = heights.size();
    double max_angle = std::atan2(dx, heights[0] - heights[1]);

    double final_max_angle = tbb::parallel_scan(
            tbb::blocked_range<int>(1, N),
            0.0,
            [&heights, &visible, dx](const tbb::blocked_range<int> &r, double max_angle, bool is_final_scan) -> double {
                for(int i = r.begin(); i != r.end(); ++i){
                    double my_angle = atan2(i * dx, heights[0] - heights[i]);
                    if(my_angle >= max_angle){
                        max_angle = my_angle;
                    }
                    else if(is_final_scan){
                        visible[i] = false;
                    }
                }
                return max_angle;
            },
            [](double a, double b){
                return std::max(a, b);
            }
        );
}

int main(){
    std::vector<int> a;
    for(int i = 0; i < 10; ++i){
        a.push_back(i);
    }
    std::vector<int> ans = a;
    std::cout << parallelPrefix(a, ans) << std::endl;
    std::vector<double> height = {10, 3, 6, 4, 9, 1, 9.9};
    std::vector<bool> vis(height.size(), true);
    visibility(height, vis, 1);
    for(auto i : vis){
        std::cout << i << " ";
    }
    std::cout << std::endl;
    return 0;
}