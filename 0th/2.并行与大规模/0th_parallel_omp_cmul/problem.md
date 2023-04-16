## 题目

小明对线性代数也感到头疼。今天，他的作业里有一道计算矩阵乘法的题目。他不知道自己的答案正不正确。他希望你能写一个程序，计算得越准确越好。由于他对于精度的要求非常高，又不希望等待程序运行花费太多时间，所以他将会把程序运行于超算上。你需要使得你编写的程序支持多线程处理从而提升处理速度。

小明遇到的题目是这样的：

给定矩阵 $A,B$, 满足 $A$ 列数与 $B$ 行数相等, 计算 $AB$.

小明再次向你介绍有个叫 OpenMP 的东西，能方便地帮助你编写并行的应用程序。你向他表明你在帮助他完成数学分析作业的时候，已经熟练运用了 OpenMP，并希望他下次能问你一些物理问题。

## 输入格式

文件引用 `rhs.h`，该头文件定义了：

```cpp
void matA(unsigned int &i, unsigned int &j, double &value);
void matB(unsigned int &i, unsigned int &j, double &value);
```

设矩阵 A 为 $\{a_{ij}\}$ ，矩阵 B 为 $\{b_{ij}\}$，那么调用这两个函数将分别给出矩阵 A 和 B 第 $i$ 行第 $j$ 列的元素的值。函数将把值放入 `value`。

程序需要接收三个命令行参数，即为 `N1`, `N2`, `N3`，表示矩阵 A 是 `N1*N2` 维的，矩阵 B 是 `N2*N3` 维的。

## 输出格式

向文件 `output.dat` 中输出结果矩阵，每行一个浮点数，按如下方式排布：（以 3x3 矩阵为例）

```txt
a00
a01
a02
a10
a11
a12
a20
a21
a22
```

一般保留 12 位有效数字即可（注意：已知问题：保留超过 12 位小数可能导致 Wrong Answer，我们的 N2 均足够大使得误差要求均不小于 $10^{-12}$）。要求误差不超过 $N_2\cdot10^{-15}$。

## 样例数据

`rhs.cpp`

```cpp
#include <cmath>
#include "rhs.h"

void matA(unsigned int &i, unsigned int &j, double &value)
{
  value = std::sin(i + 2 * j);
}

void matB(unsigned int &i, unsigned int &j, double &value)
{
  value = std::cos(3 * i + j);
}
```

`rhs.h`

```cpp
#ifndef _rhs_h_
#define _rhs_h_
void matA(unsigned int &, unsigned int &, double &);
void matB(unsigned int &, unsigned int &, double &);
#endif
```

## 提交格式

请提交一个 C++ 源码文件，我们将把它与 `rhs.h` 和 `rhs.cpp` 一同编译，编译参数为：（假设用户程序为 `omp_cmul.cpp`）

```bash
g++ -O3 -Wall -std=c++17 omp_cmul.cpp rhs.cpp -fopenmp -lm -o omp_cmul
```

  ## 测试点说明

本题一共设置 9 个测试点：

| 编号 | 分值 | 时限 | 核心 |
| ---- | ---- | ---- | ---- |
| 0    | 10   | 6s   | 6    |
| 1    | 10   | 7s   | 7    |
| 2    | 10   | 11s  | 8    |
| 3    | 10   | 6s   | 8    |
| 4    | 10   | 7s   | 6    |
| 5    | 10   | 8s   | 8    |
| 6    | 10   | 6s   | 6    |
| 7    | 10   | 7s   | 8    |
| 8    | 10   | 12s  | 8    |

同时，你需要确保你的程序足够并行，也即是说：你需要充分利用所给核心，否则可能出现 `Not So Parallel` 的结果，那样只能获得该测试点一半的分数。