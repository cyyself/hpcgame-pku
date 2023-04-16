#include <cstdio>
#include <algorithm>
#include <ctime>
#include <vector>
#include <utility>
#include <thread>
#include <sys/time.h>
#include <climits>
#include <assert.h>
// #include <execution>
#include <cstdlib>
#include <atomic>

#define data_t unsigned long long

data_t solution_simple(data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    data_t *a = (data_t *) malloc(sizeof(data_t) * n);
    for (data_t i=0;i<n;i++) {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        a[i] = gen_A;
    }
    std::sort(a,a+n);
    std::atomic<data_t> res;
    data_t block_size = n / 8;
#pragma omp parallel for
    for (int t=0;t<8;t++) {
        data_t t_res = 0;
        data_t l = block_size*t;
        data_t r = std::min(l + block_size, n);
        while (l < r) {
            t_res ^= l * a[l];
            l++;
        }
        res ^= t_res;
    }
    return res;
}

void sort_warpper(data_t *l, data_t *r) {
    return std::sort(l,r);
}

void merge_any(data_t *a, data_t *b, std::vector <std::pair<data_t, data_t> > range) {
    bool has_min_ele = true;
    while (has_min_ele) {
        has_min_ele = false;
        data_t min_ele = ULLONG_MAX;
        data_t *min_ele_ptr = NULL;
        for (auto &x: range) if (x.first <= x.second) {
            if (a[x.first] <= min_ele) {
                min_ele_ptr = &x.first;
                min_ele = a[x.first];
                has_min_ele = true;
            }
        }
        if (has_min_ele) {
            *b = min_ele;
            b++;
            (*min_ele_ptr) ++;
        }
    }
}

void parallel_merge_sort(data_t *l, data_t *r, data_t *result, int depth) {
    if (depth == 0) {
        data_t len = r - l;
        data_t mid = len / 2;
        std::thread t[2] = {
            std::thread(sort_warpper, l    , l+mid),
            std::thread(sort_warpper, l+mid, r)
        };
        for (auto &x: t) x.join();
        std::merge(l, l+mid, l+mid, r, result);
    }
    else {
        data_t len = r - l;
        data_t mid = len / 2;
        data_t *tmp_buffer = new data_t[len];
        std::thread t[2] = {
            std::thread(parallel_merge_sort, l    , l+mid, tmp_buffer    , depth-1),
            std::thread(parallel_merge_sort, l+mid,     r, tmp_buffer+mid, depth-1)
        };
        for (auto &x: t) x.join();
        std::merge(tmp_buffer, tmp_buffer+mid, tmp_buffer+mid, tmp_buffer+len, result);
        delete[] tmp_buffer;
    }
}

data_t solution_8t_merge1t(data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    data_t *a = (data_t *) malloc(sizeof(data_t) * n);
    data_t *b = (data_t *) malloc(sizeof(data_t) * n);
    for (data_t i=0;i<n;i++) {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        a[i] = gen_A;
    }
    std::vector <std::pair<data_t, data_t> > range;
    std::thread *thr[8] = {NULL};
    data_t each_thr = n / 8;
    assert(each_thr);
    data_t acc = 0;
    for (int i=0;i<8 && acc < n;i++) {
        data_t l = acc;
        data_t r = i == 7 ? (n-1) : l + each_thr - 1;
        range.emplace_back(l, r);
        thr[i] = new std::thread(sort_warpper, a+l, a+r+1);
        acc = r + 1;
    }
    for (int i=0;i<8 && thr[i];i++) thr[i]->join();
    merge_any(a, b, range);
    data_t res = 0;
    for (data_t i = 0; i < n; i ++) res ^= i * b[i];
    return res;
}

data_t solution_parallel_merge(data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    data_t *a = (data_t *) malloc(sizeof(data_t) * n);
    data_t *b = (data_t *) malloc(sizeof(data_t) * n);
    for (data_t i=0;i<n;i++) {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        a[i] = gen_A;
    }
    parallel_merge_sort(a,a+n,b,3);
    data_t res = 0;
    for (data_t i = 0; i < n; i ++) res ^= i * b[i];
    return res;
}

void data_gen(data_t *a, data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    for (data_t i=0;i<n;i++) {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        a[i] = gen_A;
    }
}

data_t solution_parallel_merge_mix(data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    data_t *a = (data_t *) malloc(sizeof(data_t) * n);
    data_t *b = (data_t *) malloc(sizeof(data_t) * n);
    std::thread gen_thread(data_gen, a, gen_A, gen_B, gen_C, n);
    data_t mid = n / 2;
    std::thread t[2] = {
        std::thread(parallel_merge_sort, a    , a+mid, b    , 2),
        std::thread(parallel_merge_sort, a+mid, a+n  , b+mid, 2)
    };
    gen_thread.join();
    for (auto &x: t) x.join();
    data_t i = 0;
    data_t res = 0;
    data_t *way1 = b    , *way1_end = b+mid;
    data_t *way2 = b+mid, *way2_end = b+n;
    while (way1 != way1_end && way2 != way2_end) {
        if (*way1 < *way2) res ^= (i++) * *way1++;
        else res ^= (i++) * *way2++;
    }
    while (way1 != way1_end) {
        res ^= (i++) * *way1++;
    }
    while (way2 != way2_end) {
        res ^= (i++) * *way2++;
    }
    return res;
}
/*
data_t solution_sort_par_unseq(data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    data_t *a = (data_t *) malloc(sizeof(data_t) * n);
    for (data_t i=0;i<n;i++) {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        a[i] = gen_A;
    }
    std::sort(std::execution::par_unseq, a, a+n);
    data_t res = 0;
    for (data_t i = 0; i < n; i ++) res ^= i * a[i];
    return res;
}
 */

void bench(data_t (*func)(data_t, data_t, data_t, data_t), data_t gen_A, data_t gen_B, data_t gen_C, data_t n) {
    timeval start;
    gettimeofday(&start, NULL);
    data_t answer = func(gen_A, gen_B, gen_C, n);
    timeval end;
    gettimeofday(&end, NULL);
    printf("%ldus, ans=%llx\n", end.tv_sec*1000000 + end.tv_usec - start.tv_sec * 1000000 - start.tv_usec, answer);
    // don't use it at midnight
}

// 3231419904646620578 2059898589516549257 2309431177327881959 100000000

int main() {
    data_t gen_A, gen_B, gen_C, n;
    scanf("%llu%llu%llu%llu", &gen_A, &gen_B, &gen_C, &n);
    
    // bench(solution_simple, gen_A, gen_B, gen_C, n);
    // bench(solution_8t_merge1t, gen_A, gen_B, gen_C, n);
    // bench(solution_parallel_merge, gen_A, gen_B, gen_C, n);
    // bench(solution_parallel_merge_mix, gen_A, gen_B, gen_C, n);
    // bench(solution_sort_par_unseq, gen_A, gen_B, gen_C, n);
    
    printf("%llu\n", solution_simple(gen_A, gen_B, gen_C, n));

    return 0;
}