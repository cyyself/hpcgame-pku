#ifndef _ANSWER_H_
#define _ANSWER_H_
#pragma GCC optimize("O3")

// kernel of resize2d_bilinear
#define resize2d_bilinear_kernel(typename)                                       \
    typename *in_data = aitisa_tensor_data(input);                               \
    typename *out_data = aitisa_tensor_data(*output);                            \
    for (int64_t i = 0; i < target_h; i++)                                       \
    {                                                                            \
        for (int64_t j = 0; j < target_w; j++)                                   \
        {                                                                        \
            double raw_u = i * (double)h / (double)target_h;                     \
            double raw_v = j * (double)w / (double)target_w;                     \
            int64_t u = (int64_t)raw_u;                                          \
            int64_t v = (int64_t)raw_v;                                          \
            if (u + 1 == h || v + 1 == w)                                        \
            {                                                                    \
                out_data[i * target_w + j] = in_data[u * w + v];                 \
                continue;                                                        \
            }                                                                    \
            typename f00 = in_data[u * w + v];                                   \
            typename f01 = in_data[u * w + v + 1];                               \
            typename f10 = in_data[(u + 1) * w + v];                             \
            typename f11 = in_data[(u + 1) * w + v + 1];                         \
            double x = raw_u - u;                                                \
            double y = raw_v - v;                                                \
            out_data[i * target_w + j] = f00 * (1 - x) * (1 - y) +               \
                                         f01 * (1 - x) * y + f10 * x * (1 - y) + \
                                         f11 * x * y;                            \
        }                                                                        \
    }

// kernel of conv2d_simple
#define CONV2D_KERNEL_SIMPLE(typename, A, B, C, N, C_in, H, W, C_out, K)                                                          \
    int H_out = H - K + 1;                                                                                                        \
    int W_out = W - K + 1;                                                                                                        \
    for (int n = 0; n < N; n++)                                                                                                   \
    {                                                                                                                             \
        for (int c = 0; c < C_out; c++)                                                                                           \
        {                                                                                                                         \
            for (int i = 0; i < H_out; i++)                                                                                       \
            {                                                                                                                     \
                for (int j = 0; j < W_out; j++)                                                                                   \
                {                                                                                                                 \
                    int offset_output = n * C_out * H_out * W_out + c * H_out * W_out + i * W_out + j;                            \
                    for (int kc = 0; kc < C_in; kc++)                                                                             \
                    {                                                                                                             \
                        for (int ki = 0; ki < K; ki++)                                                                            \
                        {                                                                                                         \
                            for (int kj = 0; kj < K; kj++)                                                                        \
                            {                                                                                                     \
                                int offset_input = n * C_in * H * W + kc * H * W + (i + ki) * W + (j + kj);                       \
                                int offset_filter = c * C_in * K * K + kc * K * K + ki * K + kj;                                  \
                                ((typename *)C)[offset_output] += ((typename *)A)[offset_input] * ((typename *)B)[offset_filter]; \
                            }                                                                                                     \
                        }                                                                                                         \
                    }                                                                                                             \
                }                                                                                                                 \
            }                                                                                                                     \
        }                                                                                                                         \
    }

// which means:
// C[n, c, i, j] += A[n, kc, i + ki, j + kj] * B[c, kc, ki, kj]

// kernel of matrix-matrix multiply
#define MM_KERNEL_SIMPLE(typename, A, B, C, M, K, N)                         \
    typename *temp_B = (typename *)malloc(K * N * sizeof(typename));         \
    for (int q = 0; q < K; ++q) for (int j = 0; j < N; ++j) {                \
        temp_B[j * K + q] = ((typename *)B)[q * N + j];                      \
    }                                                                        \
    for (int i = 0; i < M; ++i)                                              \
    {                                                                        \
        for (int j = 0; j < N; ++j)                                          \
        {                                                                    \
            for (int q = 0; q < K; ++q)                                      \
            {                                                                \
                ((typename *)C)[i * N + j] +=                                \
                    ((typename *)A)[i * K + q] * temp_B[j * K + q];          \
            }                                                                \
        }                                                                    \
    }                                                                        \
    free(temp_B);

#endif