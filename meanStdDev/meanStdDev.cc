/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "sim_timing.h"
#include "assert.h"
#include<cmath>
#include<stdlib.h>
#include<time.h>
#include<algorithm>
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include"lineartextdata/128x128_gray.h"

#define TYPE int64_t 
#define CACHE_WARMUP 5
#undef N

//Algorithm Parameters
#define row_size 128 
#define col_size 128
#define GRAY 1
#if GRAY
  #define C 1
#else 
  #define C 3
#endif
#define N row_size*col_size*C
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif

#define DEBUG 1


void meanStdDevCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double** mean, double** stdDev, double* sum, double* x){

        for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcStep; ++j){
                for(int k=0; k<srcChannels; ++k){
                    unsigned char t = src[i*srcStep + j*srcChannels + k];
                    sum[k] += t;
                }
            }
        }

        for(int k=0; k<srcChannels; ++k){
            // std::cout << sum[k] << std::endl;
            // std::cout << srcRows*srcCols << std::endl;
            *mean[k] = sum[k]/(srcRows*srcCols);
            // std::cout << *mean[k] << std::endl;

        }

        for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcStep; ++j){
                for(int k=0; k<srcChannels; ++k){
                    double t = *mean[k] - src[i*srcStep + j*srcChannels + k];
                    t *= t;
                    x[k] += t;
                }
            }
        }

        for(int k=0; k<srcChannels; ++k){
            *stdDev[k] = std::sqrt(x[k]/(srcRows*srcCols));
            // std::cout << *stdDev[k] << std::endl;
        }

}


void meanStdDev(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double** meanDSA, double** stdDevDSA, double* sumDSA, double* xDSA){

#pragma ss config
    {
	#pragma ss stream
        for(int i=0; i<srcRows; ++i){
	    //#pragma ss dfg dedicated unroll(U) //wrong computation
            for(int j=0; j<srcStep; ++j){
                for(int k=0; k<srcChannels; ++k){
                    unsigned char t = src[i*srcStep + j*srcChannels + k];
                    sumDSA[k] += t;
                }
            }
        }

        for(int k=0; k<srcChannels; ++k){
            // std::cout << sumDSA[k] << std::endl;
            // std::cout << srcRows*srcCols << std::endl;
            *meanDSA[k] = sumDSA[k]/(srcRows*srcCols);
            // std::cout << *meanDSA[k] << std::endl;

        }

	#pragma ss stream
        for(int i=0; i<srcRows; ++i){
	    //#pragma ss dfg dedicated unroll (U) //wrong computation
            for(int j=0; j<srcStep; ++j){
                for(int k=0; k<srcChannels; ++k){
                    double t = *meanDSA[k] - src[i*srcStep + j*srcChannels + k];
                    t *= t;
                    xDSA[k] += t;
                }
            }
        }

        for(int k=0; k<srcChannels; ++k){
            *stdDevDSA[k] = std::sqrt(xDSA[k]/(srcRows*srcCols));
            // std::cout << *stdDevDSA[k] << std::endl;
        }
    }
}


int main() {
  double* mean = (double*)malloc(C * sizeof(double));
  double* stddev = (double*)malloc(C * sizeof(double));
  double* meanDSA = (double*)malloc(C * sizeof(double));
  double* stddevDSA = (double*)malloc(C * sizeof(double));
  double* sumDSA = (double*)malloc(C * sizeof(double));
  double* xDSA = (double*)malloc(C * sizeof(double));
  double* sum= (double*)malloc(C * sizeof(double));
  double* x= (double*)malloc(C * sizeof(double));
  double* diffMean = (double*)malloc(C * sizeof(double));
  double* diffStddev = (double*)malloc(C * sizeof(double));
  meanStdDevCpu(image, row_size, col_size, col_size, C, &mean, &stddev, sum, x);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        meanStdDev(image, row_size, col_size, col_size, C, &meanDSA, &stddevDSA, sumDSA, xDSA);
        //meanStdDev(image, dest, ALPHA, temp, temp1);

  sumDSA[0] = 0;
  xDSA[0] = 0;

  begin_roi();
  meanStdDev(image, row_size, col_size, col_size, C, &meanDSA, &stddevDSA, sumDSA, xDSA);
  //meanStdDev(image, dest, ALPHA, temp, temp1);
  end_roi();
  sb_stats();

#if DEBUG
  for (int c = 0; c < C; c++) {
      diffMean[c] = mean[c] - meanDSA[c];
      diffStddev[c] = stddev[c] - stddevDSA[c];
      std::cout << "Ref. Mean =" << mean[c] << "\t"
                << "Result =" << meanDSA[c] << "\t"
                << "ERROR =" << diffMean[c] << std::endl;
      std::cout << "Ref. Std.Dev. =" << stddev[c] << "\t"
                << "Result =" << stddevDSA[c] << "\t"
                << "ERROR =" << diffStddev[c] << std::endl;

      if (abs(diffMean[c]) > 0.5f | abs(diffStddev[c]) > 0.5f) {
          fprintf(stderr, "ERROR: Test Failed.\n ");
          return -1;
      }
  }

  free(mean);
  free(stddev);
  free(meanDSA);
  free(stddevDSA);
  free(sumDSA);
  free(xDSA);
  free(sum);
  free(x);
  free(diffMean);
  free(diffStddev);
#endif
  return 0;
}
