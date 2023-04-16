## Problem

You've entered an AI laboratory in a university.

There are many commonly used operators in the field of deep learning, and what you need to do today is to optimize three of them.

The code of your project is included in problem attachments.


### matmul

Calculate the result of multiplying two 2D matrices. 

You can modify the macro MM_KERNEL_SIMPLE in /src/math/matmul_simple.c to optimize the operator.

If you want to test the operator yourself, you can run this

```bash
cd build
cmake ..
make
cd bin
./matmul_simple_test ../../test/test_data/matmul/2_case_32_1024_32_float/input1.dat \
    ../../test/test_data/matmul/2_case_32_1024_32_float/input2.dat \
    ../../test/test_data/matmul/2_case_32_1024_32_float/output.dat
```


### conv2d

Calculate two-dimensional convolution based on 4D input tensor(N, C_in, H, W) and 4D filter(C_in, C_out, K, K).

You can modify the macro CONV2D_KERNEL_SIMPLE in /src/nn/conv2d_simple.c to optimize the operator.

If you want to test the operator yourself, you can run this

```bash
cd build
cmake ..
make
cd bin
./conv2d_simple_test ../../test/test_data/conv2d/1_case_2_2_16_16_float/input.dat \
    ../../test/test_data/conv2d/1_case_2_2_16_16_float/filter.dat \
    ../../test/test_data/conv2d/1_case_2_2_16_16_float/output.dat
```

### resize2d_bilinear

Resize 2D images with bilinear interpolation.

You can modify the macro resize2d_bilinear_kernel in /src/nn/resize2d_bilinear.c to optimize the operator.

If you want to test the operator yourself, you can run this

```bash
cd build
cmake ..
make
cd bin
./resize2d_bilinear_test \
	../../test/test_data/resize2d_bilinear/1_case_1024_1024_512_512_float/input.dat \
	../../test/test_data/resize2d_bilinear/1_case_1024_1024_512_512_float/output.dat \
	../../test/test_data/resize2d_bilinear/1_case_1024_1024_512_512_float/shape.dat  
```

## How to submit

Put the three macros in one header file and submit it.

A sample submission: (submit this directly and you will get ~20pts)

```c
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
    for (int i = 0; i < M; ++i)                                              \
    {                                                                        \
        for (int j = 0; j < N; ++j)                                          \
        {                                                                    \
            for (int q = 0; q < K; ++q)                                      \
            {                                                                \
                ((typename *)C)[i * N + j] +=                                \
                    ((typename *)A)[i * K + q] * ((typename *)B)[q * N + j]; \
            }                                                                \
        }                                                                    \
    }

#endif
```

## How do we judge

For each operator, we have 5 test cases.

### conv2d

| Zero Score Time (secs) | Full Score Time (secs) | Full Score |
| ---------------------- | ---------------------- | ---------- |
| 0.06                   | 0.02                   | 3          |
| 0.81                   | 0.24                   | 10         |
| 0.06                   | 0.02                   | 3          |
| 0.78                   | 0.25                   | 10         |
| 0.75                   | 0.24                   | 10         |

### matmul

| Zero Score Time (secs) | Full Score Time (secs) | Full Score |
| ---------------------- | ---------------------- | ---------- |
| 0.75                   | 0.24                   | 10         |
| 0.06                   | 0.02                   | 3          |
| 0.24                   | 0.08                   | 10         |
| 3.85                   | 1.24                   | 14         |
| 1.93                   | 0.62                   | 12         |

### resize2d_bilinear

We will exclude io time.

| Zero Score Time (secs) | Full Score Time (secs) | Full Score |
| ---------------------- | ---------------------- | ---------- |
| 0.0095                 | 0.0035                 | 10         |
| 0.0165                 | 0.0057                 | 10         |
| 0.004                  | 0.0015                 | 10         |
| 0.065                  | 0.025                  | 15         |
| 0.024                  | 0.0085                 | 10         |

We use the function
$$
S_{\text{score}}(t)=\frac{\ln{\frac{t_{\text{zero\_score}}}{t}}}{\ln{\frac{t_{\text{zero\_score}}}{t_{\text{full\_score}}}}}S_{\text{full\_score}}
$$
to calculate your score.