## 题目

小明这回的确问了个“物理”些的问题。虽然依旧是一个数学问题。他还是希望你能编写一个程序，帮他解决这个问题。他依旧想把你的程序运行在超算上，并要求你的程序是并行的。

给定函数 $f(x,y)$, 正整数 $N$, 在 $N\times N$ 均匀网格上用五点格式求解 Poisson 方程
$$
\left\{
\begin{aligned}
-\Delta u(x,y)=f(x,y),\quad &(x,y)\in\Omega=(0,1)^2,\\
u(x,y)=0,\quad &(x,y)\in\partial\Omega.
\end{aligned}
\right.
$$
记 $U_{i,j}$ 为 $u(x_i,y_j)$ 的近似值, 离散格式为
$$
4U_{i,j}-U_{i-1,j}-U_{i+1,j}-U_{i,j-1}-U_{i,j+1}=\frac{1}{N^2}f(x_i,y_j),\quad x_i=\frac{i}{N},\ y_j=\frac{j}{N},\\
U_{i,j}=0,\quad (i=0)\vee(i=N)\vee(j=0)\vee(j=N).
$$
用 Gauss-Seidel 迭代求解上述线性方程组。

小明定义了一个量，名叫 EPS（各分量绝对值差最大值）。他希望你的程序能在 EPS 小于 $10^{-15}$ 后输出结果。

你看着题目苦笑了一下。如你所料，小明总是念念不忘他的 OpenMP。

## 输入格式

文件引用 `rhs.h`，该头文件定义了：

```cpp
void rhs(double &x, double &y, double &value);
```

该函数即为 $f(x,y)$。传入 $x$, $y$, $value$, 那么 $f(x,y)$ 就会被保存到 $value$ 内。

程序需要接收一个命令行参数，即为 `N`。

## 输出格式

每行一个浮点数，依次输出 $U_{0,0}$, $U_{0,1}$, ..., $U_{0,n}$, $U_{1,0}$, $U_{1,1}$, ..., $U_{n,n}$。通常保留 12 位有效数字即可。当然，我们只会检查误差大小，并不严格要求保留的小数位数。

如果误差超过 $10^{-10}$，那么该测试点将获得 0 分。误差在 $10^{-10}$ 范围之内的，误差越小，得分越高；误差在 $10^{-11}$ 范围以内的，有机会获得该测试点的全部分数。

## 样例

`rhs.cpp`

```cpp
#include <cmath>
#include "rhs.h"
#define PI25DT 3.141592653589793238462643
#define doublePIsquare 19.73920880217871723766898

void rhs(double &x, double &y, double &value) {
  value = doublePIsquare * std::sin(PI25DT * x) * std::sin(PI25DT * y);
}
```

`rhs.h`

```cpp
#ifndef _rhs_h_
#define _rhs_h_
void rhs(double &, double &, double &);
#endif
```

我们输入的 N 大约在几十的范围内。

  ## 测试点说明

本题一共设置 6 个测试点：

| 编号 | 分值 | 时限 | 核心 |
| ---- | ---- | ---- | ---- |
| 0    | 10   | 4s   | 8    |
| 1    | 20   | 6s   | 8    |
| 2    | 20   | 6s   | 8    |
| 3    | 20   | 9s   | 8    |
| 4    | 20   | 6s   | 8    |
| 5    | 30   | 15s  | 8    |

同时，你需要确保你的程序足够并行，也即是说：你需要充分利用所给核心，否则可能出现 `Not So Parallel` 的结果，那样只能获得该测试点原本得分一半的分数。