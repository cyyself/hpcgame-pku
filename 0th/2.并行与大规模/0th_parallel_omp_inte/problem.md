## 题目

小明对高等数学感到头疼。今天，他的作业里有一道计算积分的题目。他不知道自己的答案正不正确。他希望你能写一个程序，计算得越准确越好。由于他对于精度的要求非常高，又不希望等待程序运行花费太多时间，所以他将会把程序运行于超算上。你需要使得你编写的程序支持多线程处理从而提升处理速度。

小明遇到的题目转化为离散形式是这样的：

给定函数 $f(x)$, 正整数 $N$, 计算 $I_N(f)$, 其中
$$
\int_0^1f(x){\rm d}x\approx I_N(f)=\frac{1}{N}\sum_{i=1}^N f(\frac{x_i}{N}),\quad x_i=i-\frac{1}{2}.
$$

小明听说有个叫 OpenMP 的东西，能方便地帮助你编写并行的应用程序。你决定试一试。

## 输入格式

文件引用 `rhs.h`，该头文件定义了：

```cpp
void rhs(double &x, double &value);
```

即为提供的 $f(x)$。其中，`x` 将作为变量 $x$，函数将把 $f(x)$ 函数值放入 `value`。

程序需要接收一个命令行参数，即为 `N`。

## 输出格式

向文件 `output.dat` 中输出一个浮点数，通常可选择保留 12 位有效数字。与标准解答误差在 $N\cdot 10^{-15}$ 以内即算作正确。

## 样例数据

`rhs.cpp`

```cpp
#include <cmath>
#include "rhs.h"

void rhs(double &x, double &value)
{
    value = x;
}
```

`rhs.h`

```cpp
#ifndef _rhs_h_
#define _rhs_h_
void rhs(double &, double &);
#endif
```

样例输入：`1`

样例输出（`output.dat`）：`0.500000000000000` （注：只需满足精度要求，对保留的小数位数不作硬性规定）

请注意：在文件的结尾请添加一个空行（`\n`）。有选手反馈不添加空行会导致 Wrong Answer。

## 提交格式

请提交一个 C++ 源码文件，我们将把它与 `rhs.h` 和 `rhs.cpp` 一同编译，编译参数为：（假设用户程序为 `omp_cinte.cpp`）

```bash
g++ -O3 -Wall -std=c++17 omp_cinte.cpp rhs.cpp -fopenmp -lm -o omp_cinte
```

  ## 测试点说明

本题一共设置 4 个测试点：

| 编号 | 分值 | 时限 | 核心 |
| ---- | ---- | ---- | ---- |
| 1    | 10   | 16s  | 8    |
| 2    | 10   | 24s  | 8    |
| 3    | 20   | 36s  | 4    |
| 4    | 20   | 40s  | 8    |

同时，你需要确保你的程序足够并行，也即是说：你需要充分利用所给核心，否则可能出现 `Not So Parallel` 的结果，那样只能获得该测试点一半的分数。

## 提示

在`https://www.bilibili.com/video/BV18M41187ZU`可以看到高性能计算入门讲座（四）:openMP简介