#include <vector>
#include <iostream>
#include "xsimd/xsimd.hpp"
#include <algorithm>

int n, m, c, d;
float *input_raw;
float *kernel;
float *ans;

void data_input() {
    FILE *input_file = fopen("input.txt", "r");
    fscanf(input_file, "%d%d", &n, &m);
    input_raw = (float*)aligned_alloc(64, n*m*sizeof(float));
    for (int i=0;i<n;i++) {
        for (int j=0;j<m;j++) fscanf(input_file, "%f", &input_raw[i*m+j]);
    }
    FILE *kernel_file = fopen("weight.txt", "r");
    fscanf(kernel_file, "%d%d", &c, &d);
    kernel = (float*)aligned_alloc(64, c*d*sizeof(float));
    for (int i=0;i<c;i++) {
        for (int j=0;j<d;j++) {
            float x;
            fscanf(kernel_file, "%f", &x);
            kernel[i*d+j] = x;
        }
    }
    ans = new float[(n-c+1)*(m-d+1)];
}

void conv() {
    constexpr std::size_t simd_size = xsimd::simd_type<float>::size;
    assert(d % simd_size == 0);
    for (int i=0;i<(n-c+1);i++) {
        for (int j=0;j<(m-d+1);j++) {
            float x = 0;
            for (int ki=0;ki<c;ki++) {
                for (int kj=0;kj<d;kj+=simd_size) {
                    auto input_a  = xsimd::load_unaligned(&input_raw[(i+ki)*m+(j+kj)]);
                    auto kernel_a = xsimd::load_unaligned(&kernel[ki*d+kj]);
                    auto res      = input_a * kernel_a;
                    x += xsimd::reduce_add(res);
                }
            }
            ans[i*(m-d+1)+j] = x;
        }
    }
}

void data_output() {
    // freopen("output.txt", "w", stdout);
    // printf("%d %d\n",n-c+1, m-d+1);
    FILE *out_file = fopen("output.txt", "w");
    fprintf(out_file, "%d %d\n",n-c+1, m-d+1);
    for (int i=0;i<n-c+1;i++) {
        for (int j=0;j<m-d+1;j++) {
            fprintf(out_file, "%0.5f ", ans[i*(m-d+1)+j]);
        }
        fprintf(out_file, "\n");
    }
}

int main() {
    data_input();
    conv();
    data_output();
    return 0;
}