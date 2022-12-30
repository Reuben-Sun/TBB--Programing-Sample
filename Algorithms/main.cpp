#include <iostream>
#include <vector>
#include <tbb/tbb.h>

using QV = std::vector<int>;

void quickSort(QV::iterator left, QV::iterator right){
    if(left >= right){ return; }
    int pivot_value = *left;
    QV::iterator i = left, j = right - 1;
    while(i != j){
        while(i != j && pivot_value < *j) --j;    //从右向左找，直到找到一个比基准小的
        while(i != j && pivot_value >= *i) ++i;   //从左往右找，直到找到一个比基准大的
        std::iter_swap(i, j);
    }
    std::iter_swap(left, i);
    quickSort(left, i);
    quickSort(j+1, right);
}

void parallelQuicksort(QV::iterator left, QV::iterator right){
    if(left >= right){ return; }
    int pivot_value =  *left;
    QV::iterator i = left, j = right - 1;
    while (i != j) {
        while (i != j && pivot_value < *j) --j;
        while (i != j && pivot_value >= *i) ++i;
        std::iter_swap(i, j);
    }
    std::iter_swap(left, i);

    // recursive call
    tbb::parallel_invoke(
            [=]() { parallelQuicksort(left, i); },
            [=]() { parallelQuicksort(i + 1, right); }
    );

}

void parallelCutoffQuicksort(QV::iterator left, QV::iterator right){
    const int cutoff = 100;

    if (right - left < cutoff) {
        quickSort(right, left);
    }
    else {
        int pivot_value =  *left;
        QV::iterator i = left, j = right - 1;
        while (i != j) {
            while (i != j && pivot_value < *j) --j;
            while (i != j && pivot_value >= *i) ++i;
            std::iter_swap(i, j);
        }
        std::iter_swap(left, i);

        // recursive call
        tbb::parallel_invoke(
                [=]() { parallelQuicksort(left, i); },
                [=]() { parallelQuicksort(i + 1, right); }
        );
    }
}

int main() {
    std::vector<int> nums;
    for(int i = 0; i < 5000; ++i){
        nums.push_back(rand() % 5000);
    }
    tbb::parallel_for(0, 10, [](int) {
        tbb::tick_count t0 = tbb::tick_count::now();
        while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    });

    std::vector<int> nums2 = nums;
    tbb::tick_count t0 = tbb::tick_count::now();
    quickSort(nums.begin(), nums.end());
    std::cout << "Normal Time=" << (tbb::tick_count::now() - t0).seconds() << std::endl;

    tbb::tick_count t1 = tbb::tick_count::now();
    parallelQuicksort(nums2.begin(), nums2.end());
    std::cout << "Parallel Time=" << (tbb::tick_count::now() - t1).seconds() << std::endl;
    return 0;
}
