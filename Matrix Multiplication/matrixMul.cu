// Basic Matrix Multiplication 
#include <stdio.h> 
#include <stdlib.h> 
#include <ctime> 
#include <iostream> 
#include <bits/stdc++.h> 

using namespace std; 
__global__ void matrixMul (int* m, int* n, int* p, int N) { 
    //Calculate row and col 
    int row = blockIdx.y * blockDim.y + threadIdx.y; 
    int col = blockIdx.x * blockDim.x + threadIdx.x; 
    int p_sum = 0; 
    for(int i = 0; i < N; i++) { 
        p_sum += m[row * N + i] * n[i * N + col]; 
    } 
    p[row * N + col] = p_sum; 
}

void matrixMul_seq(int* m, int* n, int* p, int N) { 
    for(int i = 0; i < N; i++) { 
        for(int j = 0; j < N; j++) { 
            for(int k = 0; k < N; k++) { 
                p[i * N + j] += m[i * N + k] * n[k * N + j]; 
            } 
        } 
    } 
}

int main() { 
    //Size of square matrices 2^10 or 1024 
    //int N = 1 << 10; 
    int N = 512;
    printf("Square Matrix of Size: %d\n", N); 

    //Host Matrices 
    int* h_m; 
    int* h_n; 
    int* h_p; 
    int* h_p_seq; 
    
    //Device Matrices 
    int* d_m; 
    int* d_n; 
    int* d_p; 
    size_t bytes = N * N * sizeof(int); 
    
    //Allocate memory on host 
    h_m = (int*) malloc(bytes); 
    h_n = (int*) malloc(bytes); 
    h_p = (int*) malloc(bytes); 
    h_p_seq = (int*) malloc(bytes); 
    
    //Initialize matrices m, n, and p 
    for (int i = 0; i < N; i++) { 
        for (int j = 0; j < N; j++) { 
            h_m[i * N + j] = rand() % 1024; 
            h_n[i * N + j] = rand() % 1024; 
        } 
    } 
    
    //Allocate memory of device side 
    cudaMalloc(&d_m, bytes); 
    cudaMalloc(&d_n, bytes); 
    cudaMalloc(&d_p, bytes); 
    
    //Copy data to the device 
    cudaMemcpy(d_m, h_m, bytes, cudaMemcpyHostToDevice); 
    cudaMemcpy(d_n, h_n, bytes, cudaMemcpyHostToDevice); 
    int threads_per_block = 2; dim3 block_size (threads_per_block, threads_per_block); 
    dim3 grid_size (N / block_size.x, N / block_size.y); 

    printf("Threads per Block: %d\n", threads_per_block); 
    //printf("Block Size: %d\n", threads_per_block * threads_per_block); 
    printf("Grid Size X: %d , Grid Size Y: %d\n", grid_size.x, grid_size.y);


    matrixMul <<<grid_size, block_size>>> (d_m, d_n, d_p, N); 
    
    clock_t start, end; start = clock(); 
    matrixMul_seq(h_m, h_n, h_p_seq, N); 
    end = clock(); 
    double time_taken = double(end - start) / double(CLOCKS_PER_SEC); 
    cout << "Elapsed Time for CPU Multiplication: " << fixed << time_taken << setprecision(5); 
    cout << " sec " << endl; 
    cudaMemcpy(h_p, d_p, bytes, cudaMemcpyDeviceToHost); 
    int success = 1; 
    for(int i = 0; i < N; i++) { 
        for (int j = 0; j < N; j++) { 
            if(h_p[N * i + j] != h_p_seq[N * i + j]) { 
                printf("ERROR! \n"); 
                success = 0; 
                break; 
            }   
        }    
    } 
        
    if (success == 1) { printf("SUCCESS! \n"); } 
    free(h_m); free(h_n); free(h_p); free(h_p_seq); 
    cudaFree(d_m); cudaFree(d_n); cudaFree(d_p); 
    return (0); 
}