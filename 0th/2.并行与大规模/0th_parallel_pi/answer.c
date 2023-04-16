#include <mpi.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

double cal_range(long long n, long long l) {
    long double ans = 0;
    while (l < n) {
        double x = (l + 0.5) / n;
        ans += sqrt(1-x*x);
        l += 10;
    }
    return ans * 4 / n;
}

const long long n = 3141592654; // so magic, but it works!

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    double cur_ans = world_rank < 10 ? cal_range(n, world_rank): 0;
    double final_ans;

    MPI_Reduce(&cur_ans, &final_ans, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if (world_rank == 0) printf("%0.14lf\n", final_ans);
    MPI_Finalize();
    return 0;
}