#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <thread>

struct input_data {
    int p;
    int n;
    unsigned int arr[];
} *data_in;

struct output_data {
    unsigned int mod_ans;
    int arr[];
} *data_out;

std::mutex answer_mutex;

void cal(int l, int r) {
    if (r < l) return;
    printf("thread l=%d r=%d\n",l ,r);
    unsigned int mod_ans = 0;
    for (int i=l;i<=r;i++) {
        mod_ans = (mod_ans + data_in->arr[i]) % 100001651u;
        data_out->arr[i] = data_in->arr[i] + 1;
    }
    std::unique_lock<std::mutex> out_lock(answer_mutex);
    data_out->mod_ans = (data_out->mod_ans + mod_ans) % 100001651u;
}

int main() {
    int input_fd = open("input.bin", O_RDONLY);
    if (input_fd == -1) exit(1);
    struct stat stat_buf;
    fstat(input_fd, &stat_buf);
    data_in = (input_data*)mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, input_fd, 0);
    int n = data_in->n;
    int p = data_in->p;
    printf("n = %d, p = %d\n",n, p);

    unlink("output.bin");
    int output_fd = open("output.bin", O_RDWR | O_CREAT, (mode_t)0600);
    if (output_fd == -1) exit(1);
    int res = fallocate(output_fd, 0, 0, stat_buf.st_size - 4);
    data_out = (output_data*)mmap(NULL, stat_buf.st_size - 4, PROT_READ | PROT_WRITE, MAP_SHARED, output_fd, 0);

    int each_thread_size = n / p;

    std::thread *threads[p];
    int l = 0;
    for (int i=p-1;i>=0;i--) {
        int r = i == 0 ? (n - 1) : (l + each_thread_size - 1);
        threads[i] = new std::thread(cal, l, r);
        l = r + 1;
    }

    for (auto &x: threads) {
        x->join();
    }
    return 0;
}