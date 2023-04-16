#include "rhs.h"
#include "omp.h"
#include <cstdio>
#include <algorithm>

int n;

double *u;
double *f;

char *outbuf;

double eps = 1e-15;

int main(int argc, char *argv[]) {
    n = atoi(argv[1]);
    u = new double[(n+1)*(n+1)];
    f = new double[(n+1)*(n+1)];
    outbuf = new char[(n+1)*(n+1)*24];
#pragma omp parallel for collapse(2)
    for (int i=1;i<n;i++)
        for (int j=1;j<n;j++) {
            double x = (double)i/n;
            double y = (double)j/n;
            rhs(x, y, f[i*(n+1)+j]);
        }

    double h_sqr = (double)1 / (n * n);
    double dmax;
#ifdef DEBUG
    long long iter = 0;
#endif

    omp_lock_t dmax_lock;
    omp_init_lock(&dmax_lock);

    do {
        dmax = 0;
#pragma omp parallel for shared(u,n,dmax)
        for (int i=1;i<n;i++) {
            double sub_dmax = 0;
            for (int j=1;j<n;j++) {
                double old_value  = u[i*(n+1)+j];
                double &new_value = u[i*(n+1)+j] = 0.25 * (
                    u[(i-1)*(n+1)+j] + 
                    u[(i+1)*(n+1)+j] +
                    u[i*(n+1)+j-1] +
                    u[i*(n+1)+j+1] +
                    h_sqr * f[i*(n+1)+j]
                );
                sub_dmax = std::max(sub_dmax, std::abs(new_value - old_value));
            }
            omp_set_lock(&dmax_lock);
            dmax = std::max(dmax, sub_dmax);
            omp_unset_lock(&dmax_lock);
        }
        double dmax_out = dmax;
#ifdef DEBUG
        if (iter % 1000 == 0) printf("iter %lld, dmax = %0.10e\n", iter++, dmax_out);
        else iter ++;
#endif
    } while(dmax > eps);
#ifndef DEBUG
#pragma omp parallel for
    for (int i=0;i<=n;i++) for (int j=0;j<=n;j++) sprintf(outbuf+24*(i*(n+1)+j), "%.12e", u[i*(n+1)+j]);
    freopen("output.dat", "w", stdout);
    for (int i=0;i<(n+1)*(n+1)*24;i+=24) puts(outbuf + i);
#endif
    return 0;
}