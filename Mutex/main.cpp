#include <iostream>
#include <tbb/tbb.h>

struct bin{
    std::atomic<int> count; //4 bytes
    uint8_t padding[64 - sizeof(count)];    //60 bytes
};
struct bin2{
    //C++17后，可以用std::hardware_destructive_interference_size替代64
    alignas(64) std::atomic<int> count;
};

int main(int argc, char** argv) {

    long int n = 100000000;
    int nth = 8;
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

    //串行
    tbb::tick_count t0 = tbb::tick_count::now();
    std::for_each(image.begin(), image.end(),
                  [&](uint8_t i){hist[i]++;});
    tbb::tick_count t1 = tbb::tick_count::now();
    double t_serial = (t1 - t0).seconds();

    //并行+锁
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

    //原子操作，tbb::atomic已经废弃
    std::vector<std::atomic<int>> hist_p2(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()}, 
                [&](const tbb::blocked_range<size_t>& r)
                {
                    for(size_t i = r.begin(); i < r.end(); ++i)
                    {
                        hist_p2[image[i]]++;
                    }
                }
    );
    t1 = tbb::tick_count::now();
    double a_parallel = (t1 - t0).seconds();

    //ETS
    using vector_t = std::vector<int>;
    using priv_h_t = tbb::enumerable_thread_specific<vector_t>;
    priv_h_t priv_h{num_bins};
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
              [&](const tbb::blocked_range<size_t>& r)
              {
                priv_h_t::reference my_hist = priv_h.local();
                for (size_t i = r.begin(); i < r.end(); ++i)
                  my_hist[image[i]]++;
              });
    vector_t hist_p3(num_bins);
    /*
    for(auto i=priv_h.begin(); i!=priv_h.end(); ++i){
        for (int j=0; j<num_bins; ++j) hist_p3[j]+=(*i)[j];
    }
    */
    /*
    for (auto& i:priv_h) { // i traverses all private vectors
    std::transform(hist_p3.begin(),    // source 1 begin
                   hist_p3.end(),      // source 1 end
                   i.begin(),         // source 2 begin
                   hist_p3.begin(),    // destination begin
                   std::plus<int>() );// binary operation
    }
    */
    priv_h.combine_each([&](vector_t i){
        std::transform(hist_p3.begin(),    // source 1 begin
                   hist_p3.end(),      // source 1 end
                   i.begin(),         // source 2 begin
                   hist_p3.begin(),    // destination begin
                   std::plus<int>() );// binary operation
    });
    t1 = tbb::tick_count::now();
    double e_parallel = (t1 - t0).seconds();

    //combinable
    tbb::combinable<vector_t> priv_h2{[num_bins](){return vector_t(num_bins);}};
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                [&](const tbb::blocked_range<size_t>& r)
                {
                    vector_t& my_hist = priv_h2.local();
                    for (size_t i = r.begin(); i < r.end(); ++i)
                    my_hist[image[i]]++;
                });
    vector_t hist_p4(num_bins);
    priv_h2.combine_each([&](vector_t i)
        { // for each priv histogram a
        std::transform(hist_p4.begin(),     // source 1 begin
                        hist_p4.end(),      // source 1 end
                        i.begin(),          // source 2 begin
                        hist_p4.begin(),    // destination begin
                        std::plus<int>() ); // binary operation
        });
    t1 = tbb::tick_count::now();
    double c_parallel = (t1 - t0).seconds();

    //parallel_reduce
    using image_iterator = std::vector<uint8_t>::iterator;
    t0 = tbb::tick_count::now();
    vector_t hist_p5 = parallel_reduce (
        /*range*/    tbb::blocked_range<image_iterator>{image.begin(), image.end()},
        /*identity*/ vector_t(num_bins),
        // 1st Lambda: Parallel computation on private histograms
        [](const tbb::blocked_range<image_iterator>& r, vector_t v) {
                std::for_each(r.begin(), r.end(),
                    [&v](uint8_t i) {v[i]++;});
                return v;
            },
        // 2nd Lambda: Parallel reduction of the private histograms
        [](vector_t a, const vector_t& b) -> vector_t {
            std::transform(a.begin(),         // source 1 begin
                            a.end(),           // source 1 end
                            b.begin(),         // source 2 begin
                            a.begin(),         // destination begin
                            std::plus<int>() );// binary operation
                return a;
            });
    t1 = tbb::tick_count::now();
    double r_parallel = (t1 - t0).seconds();

    //cache padding
    std::vector<bin, tbb::cache_aligned_allocator<bin>> hist_p6(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                 [&](const tbb::blocked_range<size_t>& r)
                 {
                     for(size_t i = r.begin(); i < r.end(); ++i)
                     {
                         hist_p6[image[i]].count++;
                     }
                 }
    );
    t1 = tbb::tick_count::now();
    double pad_parallel = (t1 - t0).seconds();

    //cache padding 2
    std::vector<bin2, tbb::cache_aligned_allocator<bin2>> hist_p7(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                 [&](const tbb::blocked_range<size_t>& r)
                 {
                     for(size_t i = r.begin(); i < r.end(); ++i)
                     {
                         hist_p7[image[i]].count++;
                     }
                 }
    );
    t1 = tbb::tick_count::now();
    double pad2_parallel = (t1 - t0).seconds();

    std::cout << "Serial:       "   << t_serial   << std::endl;
    std::cout << "Parallel:     " << t_parallel << std::endl;
    std::cout << "Atomic:       " << a_parallel << std::endl;
    std::cout << "ETC:          " << e_parallel << std::endl;
    std::cout << "combinable:   " << c_parallel << std::endl;
    std::cout << "reduce:       " << r_parallel << std::endl;
    std::cout << "----cache-----" << std::endl;
    std::cout << "padding1:     " << pad_parallel << std::endl;
    std::cout << "padding2:     " << pad2_parallel << std::endl;
    // std::cout << "Speed-up: " << t_serial/t_parallel << std::endl;

    if (hist != hist_p)
        std::cerr << "Parallel computation failed!!" << std::endl;
    return 0;
}
