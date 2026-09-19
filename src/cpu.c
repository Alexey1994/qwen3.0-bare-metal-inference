void matmul_optimized(Float32* C, Float32* A, Float32* B, Number M, Number K, Number N)
{
	Number i, j, k;

	for(i = 0; i < M; ++i) {
		for(j = 0; j < N; ++j) {
			Float32 sum = 0;
			for(k = 0; k < K; ++k) {
				sum += A[i * K + k] * B[j * K + k];
			}
			C[i * N + j] = sum;
		}
	}
}


// C = A * B^T
// A[M*K]
// B[N*K]
// C[M*N]