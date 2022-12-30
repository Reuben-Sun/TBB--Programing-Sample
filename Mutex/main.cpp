#include <iostream>
#include <tbb/tbb.h>

int main(int argc, char** argv) {

    long int n = 100000000;
    int nth = 4;
    constexpr int num_bins = 256;

    // Initialize random number generator
    std::random_device seed;    // Random device seed
    std::mt19937 mte{seed()};   // mersenne_twister_engine
    std::uniform_int_distribution<> uniform{0,num_bins};
    // Initialize image
    std::vector<uint8_t> image; // empty vector
    image.reserve(n);           // image vector prealocated
    std::generate_n(std::back_inserter(image), n,
                    [&] { return uniform(mte); }
    );
    // Initialize histogram
    std::vector<int> hist(num_bins);

    //tbb::task_scheduler_init init{nth}已经弃用
    tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, nth);

    // Serial execution
    tbb::tick_count t0 = tbb::tick_count::now();
    std::for_each(image.begin(), image.end(),
                  [&](uint8_t i){hist[i]++;});
    tbb::tick_count t1 = tbb::tick_count::now();
    double t_serial = (t1 - t0).seconds();

    // Parallel execution
    using my_mutex_t=tbb::spin_mutex;
    std::vector<my_mutex_t> fine_m(num_bins);
    std::vector<int> hist_p(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                 [&](const tbb::blocked_range<size_t>& r)
                 {
                     for (size_t i = r.begin(); i < r.end(); ++i){
                         int tone=image[i];
                         my_mutex_t::scoped_lock my_lock{fine_m[tone]};
                         hist_p[tone]++;
                     }
                 });
    t1 = tbb::tick_count::now();
    double t_parallel = (t1 - t0).seconds();

    std::cout << "Serial: "   << t_serial   << ", ";
    std::cout << "Parallel: " << t_parallel << ", ";
    std::cout << "Speed-up: " << t_serial/t_parallel << std::endl;

    if (hist != hist_p)
        std::cerr << "Parallel computation failed!!" << std::endl;
    return 0;
}
