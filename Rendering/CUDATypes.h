#pragma once

#include <cstdio>
#include <cuda_runtime_api.h>

#define A_CUDA_CPUGPU __device__ __host__ 
#define A_CUDA_CPU	__host__ 
#define A_CUDA_GPU	__device__ 
#define A_CUDA_KERNEL __global__ 

#if defined(__CUDACC__) // NVCC
   #define A_CPUGPU_ALIGN(n) __align__(n)
#elif defined(__GNUC__) // GCC
  #define A_CPUGPU_ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER) // MSVC
  #define A_CPUGPU_ALIGN(n) __declspec(align(n))
#else
  #error "Please provide a definition for A_CPUGPU_ALIGN macro for your host compiler!"
#endif

#define CUDA_CHECK_ERROR(...) TestCUDAError(__VA_ARGS__, __FILE__, __LINE__);
	
#pragma warning( disable : 26812 )

static void TestCUDAError(cudaError_t err, const char* file, const unsigned int line)
{
	if (err != cudaError::cudaSuccess)
	{
		printf("CudaError (file = %s, line = %i): %s\n", file, line, cudaGetErrorString(err));
		err = cudaGetLastError();
	}
}

class CUDAManaged {
public:
  void *operator new(const size_t len) {
	void *ptr;
	CUDA_CHECK_ERROR(cudaMallocManaged(&ptr, len));
	cudaDeviceSynchronize();
	return ptr;
  }

  void operator delete(void* ptr) {
	cudaDeviceSynchronize();
	cudaFree(ptr);
  }
};