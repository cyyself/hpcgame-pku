#include <cstdio>
#include "rhs.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>
#include <omp.h>

const int tile_size = 32;

long long n1, n2, n3;

double *mtxa;   // [n1][n2]
double *mtxb_t; // [n3][n2]

char *printbuf;

int main(int argc, char *argv[]) {
    n1 = atoll(argv[1]);
    n2 = atoll(argv[2]);
    n3 = atoll(argv[3]);

    mtxa = new double[n1*n2];
    mtxb_t = new double[n2*n3];
    printbuf = new char[n1*n3*24];

    omp_set_num_threads(std::max(std::thread::hardware_concurrency(),1u));

#pragma omp parallel for
    for (unsigned int i=0;i<n1;i++)
        for (unsigned int j=0;j<n2;j++) {
            matA(i, j, mtxa[i*n2+j]);
        }

#pragma omp parallel for
    for (unsigned int i=0;i<n3;i++)
        for (unsigned int j=0;j<n2;j++) {
            matB(j, i, mtxb_t[i*n2+j]);
        }
    
#pragma omp parallel for collapse(2) schedule(dynamic)
    for (long long t_i=0;t_i<n1;t_i+=tile_size)
        for (long long t_j=0;t_j<n3;t_j+=tile_size) {
            for (long long i=t_i;i<std::min(n1, t_i+tile_size);i++)
                for (long long j=t_j;j<std::min(n3, t_j+tile_size);j++) {
                    double ans = 0;
                    for (int k=0;k<n2;k++) {
                        ans += mtxa[i*n2+k] * mtxb_t[j*n2+k];
                    }
                    sprintf(printbuf+24*(i*n3+j), "%.11e", ans);
                }
        }

    freopen("output.dat", "w", stdout);
    for (long long i=0;i<n1*n3;i++) {
        puts(printbuf + 24*i);
    }
    return 0;
}