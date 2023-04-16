#include <cstdio>
#include "rhs.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>

std::mutex answer_mutex;
double ans;

long long n;

void cal(long long l, long long r) {
    if (r < l) return;
    printf("thread l=%lld r=%lld\n",l ,r);
    double sum = 0;
    for (long long i=l;i<=r;i++) {
        double x = ((double)i - 0.5) / n;
        double cur;
        rhs(x, cur);
        sum += cur;
    }
    std::unique_lock<std::mutex> out_lock(answer_mutex);
    ans += sum;
}

int main(int argc, char *argv[]) {
    n = atoll(argv[1]);
    const unsigned int nr_cpus = std::max(std::thread::hardware_concurrency(),1u);
    long long each_size = n / nr_cpus;

    std::thread *threads[nr_cpus];

    long long l = 1;
    for (int i=nr_cpus-1;i>=0;i--) {
        long long r = i == 0 ? (n) : (l + each_size - 1);
        threads[i] = new std::thread(cal, l, r);
        l = r + 1;
    }

    for (auto &x: threads) {
        x->join();
    }

    freopen("output.dat","w",stdout);
    printf("%0.15lf\n", ans / n);
    return 0;
}