#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct lens_t {
    float x, y, r, d;
};

struct pane_t {
    int panecount;
    float *panepos;
    int *lensindex;
    lens_t *lensdata;
};

struct result_t {
    int raydensity;
    int sensordensity;
    int *sensordata;
};

int loadconf(const char *fn, pane_t &pane, result_t &result) {
    size_t n;
    FILE *fi;
    if (fi = fopen(fn, "rb")) {
        n = fread(&result.raydensity, 4, 1, fi);
        n = fread(&result.sensordensity, 4, 1, fi);
        // result.sensordata = new std::atomic<int>[result.sensordensity*result.sensordensity];
        cudaMallocManaged(&result.sensordata, result.sensordensity*result.sensordensity * sizeof(int));
        cudaMemset(&result.sensordata, 0, result.sensordensity*result.sensordensity * sizeof(int));
        n = fread(&pane.panecount, 4, 1, fi);
        // pane.panepos = (float *)malloc(pane.panecount * sizeof(float));
        cudaMallocManaged(&pane.panepos, pane.panecount * sizeof(float));
        n = fread(pane.panepos, 4, pane.panecount, fi);
        // pane.lensindex = (int *)malloc((pane.panecount + 1) * sizeof(int));
        cudaMallocManaged(&pane.lensindex, (pane.panecount + 1) * sizeof(int));
        n = fread(pane.lensindex, 4, pane.panecount + 1, fi);
        // pane.lensdata = (lens_t *)malloc(pane.lensindex[pane.panecount] * sizeof(lens_t));
        cudaMallocManaged(&pane.lensdata, pane.lensindex[pane.panecount] * sizeof(lens_t));
        n = fread(pane.lensdata, 16, pane.lensindex[pane.panecount], fi);
        fclose(fi);
    }
    printf("%lu\n", n);
    return 0;
}

__global__ void causcal_kernel(pane_t pane, result_t result) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    float rs = 1.0f / result.raydensity;
    long long tot = 1ll * result.raydensity * result.raydensity;
    for (long long step=index;step<tot;step+=stride) {
        int i = step / result.raydensity;
        int j = step % result.raydensity;
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
            if (x <= 0.0f || x >= 1.0f || y <= 0.0f || y >= 1.0f)
            {
                goto next;
            }
            z = pane.panepos[k];

            float gx = 0.0f, gy = 0.0f;
            float rx, ry;
            int is = pane.lensindex[k];
            int ie = pane.lensindex[k + 1];
            for (int l = is; l < ie; l++)
            {
                rx = x - pane.lensdata[l].x;
                ry = y - pane.lensdata[l].y;
                float r = rx * rx + ry * ry;
                float invr02 = pane.lensdata[l].r * pane.lensdata[l].r;
                if (r < invr02 * 0.99999f)
                {
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
            atomicAdd(&result.sensordata[pixelindex], 1);
        }
        next:;
    }
}

const int nr_blocks = 4096;
const int nr_threads = 512;

int causcal(pane_t pane, result_t result) {
    causcal_kernel<<<nr_blocks, nr_threads>>>(pane, result);
    cudaDeviceSynchronize();
    return 0;
}
int main() {
    result_t result;
    pane_t pane;
    loadconf("./conf.data", pane, result);
    causcal(pane, result);
    FILE *fi;
    if (fi = fopen("./out.data", "wb")) {
        fwrite(result.sensordata, 1, result.sensordensity * result.sensordensity * sizeof(int), fi);
        fclose(fi);
    }
    return 0;
}