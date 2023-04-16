## 任务背景
焦散是光线经过透明物体后在某些位置汇聚形成的图案，因为其空间频率远高于单纯漫反射下的明暗，在渲染时需要较大的计算量。考虑一个简单的场景，一束方形截面的平行单色光经过若干方向玻璃板照在方形屏幕上。在这个场景中，只需要采样平行光最终到达屏幕的位置，即可得到焦散图案。

## 物理模型
这里我们假设玻璃板会让经过的光线发生不均匀的相位落后，从而向不同方向折射，为了简化计算，忽略玻璃板的厚度。相位落后由多个定义在半径为r的圆形区域内的，最大值为d的旋转对称模板函数相加。 模板函数具有如下形式  
$f(t=\frac{\sqrt{x^2+y^2}}{r}) = d*exp(-\frac{4t^2}{1-t^2})$  
样例程序中包含计算模板函数梯度的部分，可以根据自己的需要修改。  

## 输入与输出
题目数据量较大，因此将使用读写二进制文件的输入输出方式。处理输入输出的代码已经包含在样例程序中，可以根据自己的需要修改。

## 任务目标
你需要适当修改提供的在cpu上进行计算的样例程序，使其运行在gpu上。如果你直接提交样例程序虽然能够获得正确的结果但会运行相当长的时间。样例程序中会包含一个没用的kernel，用于证明这个程序确实在gpu上运行了。  
务必确保全程使用单精度浮点计算，允许的误差仅考虑到GPU与CPU浮点运算差异，单双精度浮点差别远大于此。  

## 有用的信息
r在0.05到0.10的范围内随机分布  
模板函数中心位置在方形区域内随机分布  
可以使用`int atomicAdd(int* address, int val);`进行计数。  
编译命令`nvcc -O3`  
运行在单核，八分之一个A100上  
共两个测试点：  

| 测试点 ID | 玻璃板数量 | 每个玻璃板上的模板函数数量 | 采样数量 | 输出像素 | 分值 |
| - | - | - | - | - | - |
| 0 | 2 | 256 | 4096*4096 | 256*256 | 10 |
| 1 | 32 | 256 | 16384*16384 | 1024*1024 | 190 |

附件中提供一份输入文件，可以用于本地调试。  

## 评分标准
如果能在目标时间内得到正确的焦散图案可以得到基本分。  
运行时间越短得到的附加分越高。  

## 样例程序
```
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct lens_t {
    float x, y, r, d;
};

struct pane_t {
    int panecount;
    float* panepos;
    int* lensindex;
    lens_t* lensdata;
};

struct result_t {
    int raydensity;
    int sensordensity;
    int* sensordata;
};

__global__ void ker_test(float* x, int n) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i < n) {
        x[i] = i * i;
        printf("%f\n", x[i]);
    }
}

int cuinit() {
    float* temp;
    cudaMalloc(&temp, 128);
    ker_test << <1, 32 >> > (temp, 32);
    cudaFree(temp);
    return 0;
}

int causcal(pane_t pane, result_t result) {
    memset(result.sensordata, 0, result.sensordensity * result.sensordensity * sizeof(int));
    float rs = 1.0f / result.raydensity;

    for (int i = 0; i < result.raydensity; i++) {
        for (int j = 0; j < result.raydensity; j++) {

            float x, y, z, kx, ky, invkz;
            y = (i + 0.5f) * rs;
            x = (j + 0.5f) * rs;
            z = 0.0f;
            kx = 0.0f;
            ky = 0.0f;
            invkz = 1.0f;

            for (int k = 0; k < pane.panecount; k++) {
                x += (pane.panepos[k] - z) * kx * invkz;
                y += (pane.panepos[k] - z) * ky * invkz;
                if (x <= 0.0f || x >= 1.0f || y <= 0.0f || y >= 1.0f) {
                    goto next;
                }
                z = pane.panepos[k];

                float gx = 0.0f, gy = 0.0f;
                float rx, ry;
                int is = pane.lensindex[k];
                int ie = pane.lensindex[k + 1];
                for (int l = is; l < ie; l++) {
                    rx = x - pane.lensdata[l].x;
                    ry = y - pane.lensdata[l].y;
                    float r = rx * rx + ry * ry;
                    float invr02 = pane.lensdata[l].r * pane.lensdata[l].r;
                    if (r < invr02 * 0.99999f) {
                        invr02 = 1.0f / invr02;
                        r = r * invr02;
                        r = 4.0f / (1.0f - r);
                        r = -0.5f * exp(4.0f - r) * r * r * invr02 * pane.lensdata[l].d;
                        gx += r * rx;
                        gy += r * ry;
                    }
                }

                kx += gx;
                ky += gy;
                float kp = kx * kx + ky * ky;
                if (kp >= 1.0f) {
                    goto next;
                }
                invkz = 1.0f / sqrt(1.0f - kp);
            }

            x += (1.0f - z) * kx * invkz;
            y += (1.0f - z) * ky * invkz;

            if (x > 0.0f && x < 1.0f && y > 0.0f && y < 1.0f) {
                int pixelindex = int(x * result.sensordensity) + result.sensordensity * int(y * result.sensordensity);
                result.sensordata[pixelindex]++;
            }

        next:
            ;
        }
    }
    return 0;
}

int loadconf(const char* fn, pane_t& pane, result_t& result) {
    size_t n;
    FILE* fi;
    if (fi = fopen(fn, "rb")) {
        n = fread(&result.raydensity, 4, 1, fi);
        n = fread(&result.sensordensity, 4, 1, fi);
        result.sensordata = (int*)malloc(result.sensordensity * result.sensordensity * sizeof(int));

        n = fread(&pane.panecount, 4, 1, fi);
        pane.panepos = (float*)malloc(pane.panecount * sizeof(float));
        n = fread(pane.panepos, 4, pane.panecount, fi);
        pane.lensindex = (int*)malloc((pane.panecount + 1) * sizeof(int));
        n = fread(pane.lensindex, 4, pane.panecount + 1, fi);
        pane.lensdata = (lens_t*)malloc(pane.lensindex[pane.panecount] * sizeof(lens_t));
        n = fread(pane.lensdata, 16, pane.lensindex[pane.panecount], fi);

        fclose(fi);
    }
    printf("%lu\n", n);
    return 0;
}

int main() {
    result_t result;
    pane_t pane;
    loadconf("./conf.data", pane, result);

    causcal(pane, result);
    
    FILE* fi;
    if (fi=fopen("./out.data", "wb")) {
        fwrite(result.sensordata, 1, result.sensordensity * result.sensordensity * sizeof(int), fi);
        fclose(fi);
    }

    return 0;
}
```