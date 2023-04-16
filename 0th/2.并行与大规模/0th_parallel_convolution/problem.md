## 背景

二维卷积是卷积神经网络的基础操作，可以提取数据特征，在图像识别领域应用广泛。卷积的复杂度导致训练困难，也是图像识别领域的重要瓶颈之一。

对卷积感兴趣的同学，可以去翻一翻 Ian Goodfellow 等人写的`Deep Learning`的第9章。我们只引用书中的一张图介绍卷积的计算。

对于矩阵`Input`，我们使用一个比它规模小的多的`Kernel`进行运算。本题我们方法大概是这样：

设`Input`是$A * B$矩阵，`Kernel`是$ C * D$矩阵，对于每个位置`(i, j)`，如果可以取一个和`Kernel`大小一样的不越界矩阵(本题不考虑padding)，我们就把这个矩阵中与`Kernel`中对应元素乘起来并求和，结果放在$Output_{(i, j)}$中。
$$
\left[
\begin{matrix}
   I_{(i, j)} & I_{(i, j+2)} & .... & I_{(i, j+D)} \\
   I_{(i+1, j)} & I_{(i+1, j+2)} & .... & I_{(i+1, j+D)} \\
   .... & .... & .... & ....\\
   I_{(i+C, j)} & I_{(i+C, j+2)} & ... & I_{(i+C, j+D)} \\
  \end{matrix}
 \right]
$$
计算完所有可算的$i$, $j$，我们就得到了卷积的输出。
$$
Output_{(i, j)} = \sum_{p = 0}^{C-1} \sum_{q = 0}^{D-1} Input_{(i+p, j+q)} * Kernel_{(p, q)}
$$
下图引自Ian Goodfellow 等人著作Deep Learning`，很形象地表现了这一过程。因为元素运算是可向量化的，我们试图在CPU上使用SIMD（单指令多数据）的方法，优化二维卷积运算。

![2dconv](https://hpcgame.pku.edu.cn/oss/images/conv/2dconv.png)



## 要怎么做才好呢

要不，问问chatGPT？

他说的确实对，向量化的思路没错，但是我们不打算给你太多的核心，所以你得让每一条指令都能操作多个数据。

<div class="row">
  <div class="column">
   <center>
   <img src="https://hpcgame.pku.edu.cn/oss/images/conv/chat1.png" alt="图1" style="width:50%;" />
   </center>
  </div>
  <div class="column">
   <center>
    <img src="https://hpcgame.pku.edu.cn/oss/images/conv/chat2.png" alt="图2" style="width:50%;" />
   </center>
  </div>
</div>





## 硬件条件

评测平台的CPU是Intel® Xeon® Platinum 8358，关闭超线程。

每次评测分配1CPU核心，4G内存。

CPU相关参数如下：

32K数据L1缓存，1.25MB L2缓存。

指令集扩展：Intel® SSE4.2, Intel® AVX, Intel® AVX2, Intel® AVX-512

## 评分标准

出题人很讲武德，你只要跑得比chatGPT指导他写的`Python`程序快就行了。不过，出题人使用了Intel编译的加速优化后的`Python`和`numpy`，所以你大概要写`C++`才能拿到看起来比较多的分数。

对于性能评测，每个测试点15分，$max(0, min((T(出题人程序)/T(你的程序) - 0.8) / 2 * 15, 15))$就是你的得分。



## 提交说明

我们下发的包是一个`cmake`工程的骨架。改里面的`answer.cpp`就行。

编译方式：运行`compile.sh`。

提交方式：直接提交`answer.cpp`。如果您需要使用其他语言，请与我们联系`hpcgame@pku.edu.cn`。

## 输入与输出说明

本题所有数字均为单精度浮点数，允许的浮点误差是$1*10^{-5}$

### 输入文件：

从`input.txt`读入$Input$，第一个数字是行数$N$，第二个数字是列数$M$。后面是$N$行数据，每一行$M$个数字。

从`weight.txt`读入$Kernel$，第一个数字是$C$，第二个数字是$D$。后面是$C$行数据，每一行$D$个数字。

### 输出文件：

把你得到的$Output$输出到`output.txt`，第一个数字是行数$P$，第二个数字是列数$Q$，后面有$P$行，每行$Q$个数字。

## 一些提示

1. 输入的卷积核边长是16的倍数，因为AVX512每次可以操作16个float
2. 没有padding！$16 * 16$的矩阵和$2 * 2$的$Kernel$输出会是$15 * 15$的
3. 建议多注意数据对齐的问题，64 bytes对齐可能会是比较好的选择，可以参考Intel的[这篇文章](https://www.intel.com/content/www/us/en/developer/articles/technical/data-alignment-to-assist-vectorization.html)。评测使用C++ 17标准，其中`aligned_alloc`挺适合完成这个工作的。
4. 请关注缓存命中的问题，比较影响性能。

## 零基础友好部分

1. 附件有ETH的介绍SIMD的slides，里面例子很多，强烈推荐。
2. [xsimd](https://github.com/xtensor-stack/xsimd)这个`C++`wraper挺好用的，所以我们帮你预装好了。
3. Intel的手册是重要资料，在[这里](https://www.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-avx-512-instructions.html)。
4. 知乎上有个[中文教程](https://zhuanlan.zhihu.com/p/591900754)，我们觉得还不错。

## 测试数据规模

| 数据点 | Input     | Kernel | 分值（给分方式） |
| ------ | --------- | ------ | ---------------- |
| 0      | 512x1024  | 64x64  | 15（性能评测）   |
| 1      | 512x1024  | 32x32  | 15（性能评测）   |
| 2      | 512x1024  | 16x16  | 15（性能评测）   |
| 3      | 2048x2048 | 64x64  | 15（性能评测）   |
| 4      | 2048x2048 | 32x32  | 15（性能评测）   |
| 5      | 2048x2048 | 16x16  | 15（性能评测）   |
| 6      | 1024x2048 | 64x64  | 15（性能评测）   |
| 7      | 1024x2048 | 32x32  | 15（性能评测）   |
| 8      | 1024x2048 | 16x16  | 15（性能评测）   |
| 9      | 1024x1024 | 64x64  | 15（性能评测）   |
| 10     | 1024x1024 | 32x32  | 15（性能评测）   |
| 11     | 1024x1024 | 16x16  | 15（性能评测）   |
| 12     | 4096x4096 | 16x16  | 10（不超时即可） |
| 13     | 4096x4096 | 32x32  | 10（不超时即可） |