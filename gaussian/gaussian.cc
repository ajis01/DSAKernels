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
#define CACHE_WARMUP 2
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
inline
double exp2(double x) {
  x = 1.0 + x / 1024;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x;
  return x;
}
void gaussianCpu(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		    TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){

	double den = 1/sqrt(2*M_PI);
	for(int r=range; r<src1Rows-range; r++) {
            for(int c=range; c<src1Step-range; c++) {
                double res = 0;
                int index = (r-range)*src1Step + (c-range);
                for(int x = -range; x<=range; x++) {
                    double m = den*exp2(-0.5*x*x);
                    // std::cout << index << " " << r << " " << c << std::endl;
                    res += m * src1[index];
                }
                src2[index] = res;
            }
        }

        for(int r=range; r<src1Rows-range; r++) {
            for(int c=range; c<src1Step-range; c++) {
                double res = 0;
                int index = (r-range)*src1Step + (c-range);
                for(int x = -range; x<=range; x++) {
                    double m = den*exp2(-0.5*x*x);
                    res += m * src2[index];
                }
                dest[index] = res;
            }
        }
}


void gaussian(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		    TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){

#pragma ss config
    {
	double den = 1/sqrt(2*M_PI);
	for(int r=range; r<src1Rows-range; r++) {
            for(int c=range; c<src1Step-range; c++) {
                double res = 0;
                int index = (r-range)*src1Step + (c-range);
                for(int x = -range; x<=range; x++) {
                    double m = den*exp2(-0.5*x*x);
                    // std::cout << index << " " << r << " " << c << std::endl;
                    res += m * src1[index];
                }
                src2[index] = res;
            }
        }

        for(int r=range; r<src1Rows-range; r++) {
            for(int c=range; c<src1Step-range; c++) {
                double res = 0;
                int index = (r-range)*src1Step + (c-range);
                for(int x = -range; x<=range; x++) {
                    double m = den*exp2(-0.5*x*x);
                    res += m * src2[index];
                }
                dest[index] = res;
            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  int range = 3;
  gaussianCpu(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, range);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        gaussian(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
        //gaussian(image, dest, ALPHA, temp, temp1);


//  for(int i=0; i<row_size; ++i){
//    for(int j=0; j<col_size; ++j){
//      dest[i*col_size + j] = 0;
//      //dest1[i*col_size + j] = 0;
//    }
//  }
//
//  begin_roi();
//  gaussian(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
//  //gaussian(image, dest, ALPHA, temp, temp1);
//  end_roi();
//  sb_stats();

#if DEBUG
  double err_cnt = 0;
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      if(dest[i*col_size + j] != dest1[i*col_size + j]){
              err_cnt++;
              //std::cout << dest[i*col_size + j] << ":" << dest1[i*col_size + j] << std::endl;
      }
      //      std::cout << dest[i*col_size + j] << std::endl;
    }
  }
  
  std::cout << "ERROR percent = " << err_cnt/(row_size*col_size*C) << std::endl;
  std::cout << "ERROR count = " << err_cnt << " out of " << row_size*col_size*C << std::endl;
#endif
  return 0;
}
