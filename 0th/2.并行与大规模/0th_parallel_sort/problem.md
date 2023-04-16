## 题面

道生一，一生二，二生三，三生万物。

我们给你选择了三个“魔力数字”`genA`, `genB`, `genC`，请你使用这三个数生成$n$个“随从数字”并排序，得到`data`数组。经过对`data`数组的计算，你会得到一个`归一数`，把这个数输出到标准输出即可。
（本题中的所有数字都是64位无符号整数。）

## 输入

从标准输入中读入四个数字，分别是`genA`, `genB`, `genC`,`n` 。

## 输出

输出一个`归一数`到标准输出，生成方式见下文。

## 数据生成与测试

```cpp
#define data_t unsigned long long
/*
data数组的生成的示意代码如下：
*/
data_t gen_next(){
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C ++;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        return gen_A;        
}
void data_init(){
        scanf("%llu%llu%llu%llu", &gen_A, &gen_B, &gen_C, &n);
        a = (data_t *) malloc(sizeof(data_t) * n);
    	for (data_t i = 0 ; i < n; i ++) {
            a[i] = gen_next();
        }
}
/*
归一数的计算方式如下：
*/
data_t get_res(data_t *a,data_t n){
        for (data_t i = 0,tmp; i < n; i ++)
            for (data_t j = 0; j < n - 1 - i; j ++) {
                if (a[j] > a[j + 1]) {
                    tmp = a[j];
                    a[j] = a[j+1];
                    a[j+1] = tmp;
                }
            }
        data_t res = 0;
        for (data_t i = 0; i < n; i ++) {
            res ^= i * a[i];
        }
        return res;
}
#undef data_t
```

其中`i * a[i]`的溢出处理方式是低64位截断。

## 评测说明

评测提供8个CPU核心，32G内存。

本题共两个测试点，分别为 80 分和 120 分，都会根据您程序运行的时间给出最终得分。程序运行越快，得分越高。其中第一个数据点用时 9.8s 以内开始得分，0.45s 满分；第二个数据点 53.0s 以内开始得分，3.0s 满分。这个标准是依照题给例程和组委会已知的最优优化给出的。

## 提示与说明

1. 除了打表和攻击比赛平台等不正当的方式，你可以使用任何优化方式 。不仅仅是排序部分，数据生成和最后生成res过程或许都能优化，正确性角度只要得到的`归一数`正确即可。

2. 本题的提交方式是，将二进制文件解答文件（统一命名为 `answer`，请编译到 `linux-x86_64` （有时也称 `amd64`））和代码、编译脚本一起打包为压缩包提交（请注意：`answer`应放置在压缩包根目录，而非任何子目录下；否则评测时无法识别），平台直接调用二进制文件进行评测。对于所有获奖选手，我们会逐一对代码进行审核、编译。如不一致且不能说明原因，将按照违规取消本题成绩，并对其他题目严格审查。

3. 本题还是有梯度的，可以拿挺多部分分，have fun!

4. 如果你的评测结果是 Internal Error，并且报错 exit status 126，请你检查你是否将 `answer` 程序放置在了解答压缩包的根目录（注意：不应该在某个子文件夹内）！

5. 参考资料 <https://axelle.me/2022/04/19/diverting-lsd-sort/>