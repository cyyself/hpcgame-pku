#include <cufft.h>
#include <complex>

void my_fft(int c, int r, std::complex<double> *in, std::complex<double> *out) {
    cufftHandle plan;
    cufftPlan3d(&plan, c, c, c, CUFFT_Z2Z);
    cufftExecZ2Z(plan, reinterpret_cast<cufftDoubleComplex*>(in),
                 reinterpret_cast<cufftDoubleComplex*>(out),
                 CUFFT_INVERSE);
    cufftDestroy(plan);
}